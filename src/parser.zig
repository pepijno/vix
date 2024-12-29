const std = @import("std");

const ast = @import("ast.zig");
const AstProperty = ast.AstProperty;
const AstElement = ast.AstElement;
const lex = @import("lexer.zig");
const Lexer = lex.Lexer;
const LexTokenType = lex.LexTokenType;
const Token = lex.Token;
const util = @import("util.zig");
const Location = util.Location;
const Type = @import("types.zig").Type;

fn vsynerror(allocator: std.mem.Allocator, token: Token, args: std.ArrayList(LexTokenType)) anyerror!noreturn {
    const stderr = std.io.getStdErr().writer();

    try stderr.print("{s}:{}:{}: syntax error: expected ", .{
        util.sources.items[token.location.file],
        token.location.line_number,
        token.location.column_number,
    });

    var i: usize = 0;
    while (i < args.items.len) : (i += 1) {
        const token_type = args.items[i];
        try stderr.print("{s}, ", .{@tagName(token_type)});
    }

    try stderr.print("found '{s}'\n", .{@tagName(token.type)});
    try token.location.errorLine(allocator);
    std.process.exit(1);
}

fn synassert(allocator: std.mem.Allocator, token: Token, condition: bool, args: std.ArrayList(LexTokenType)) anyerror!void {
    if (!condition) {
        try vsynerror(allocator, token, args);
    }
}

pub const Parser = struct {
    allocator: std.mem.Allocator,
    next_id: usize,
    properties: std.AutoHashMap(usize, *AstProperty),
    lexer: *Lexer,

    pub fn init(allocator: std.mem.Allocator, lexer: *Lexer) @This() {
        return .{
            .allocator = allocator,
            .next_id = 1,
            .properties = std.AutoHashMap(usize, *AstProperty).init(allocator),
            .lexer = lexer,
        };
    }

    pub fn parse(self: *@This()) !*AstElement {
        const root = try self.parseRoot();
        try self.expectToken(.eof, null);
        return root;
    }

    fn parseRoot(self: *@This()) !*AstElement {
        var root = try self.allocator.create(AstElement);
        root.value = .{
            .properties = std.ArrayList(*AstProperty).init(self.allocator),
        };

        while (true) {
            const property = try self.parseProperty();
            if (property == null) {
                break;
            }
            try root.value.properties.append(property.?);
        }

        return root;
    }

    fn parseProperty(self: *@This()) !?*AstProperty {
        var token = Token.initEmpty();
        const token_type = try self.lexer.lex(&token);
        if (token_type != .name) {
            self.lexer.unlex(&token);
            return null;
        }

        var property = try self.allocator.create(AstProperty);
        property.id = self.next_id;
        self.next_id += 1;
        property.name = try self.allocator.dupe(u8, token.value.name);
        try self.expectToken(.assign, null);
        property.value = try self.parseElement();
        try self.expectToken(.semicolon, null);

        try self.properties.put(property.id, property);

        return property;
    }

    fn parseElement(self: *@This()) anyerror!*AstElement {
        var token = Token.initEmpty();
        var element = try self.allocator.create(AstElement);
        element.value = .{
            .integer = 0,
        };

        const token_type = try self.lexer.lex(&token);
        switch (token_type) {
            .name => {
                element.value = .{
                    .property_access = std.ArrayList([]const u8).init(self.allocator),
                };
                const name = try self.allocator.dupe(u8, token.value.name);
                try element.value.property_access.append(name);
                var token2 = Token.initEmpty();
                loop: while (true) {
                    const token_type2 = try self.lexer.lex(&token2);
                    switch (token_type2) {
                        .dot => {
                            try self.expectToken(.name, &token2);
                            const name2 = try self.allocator.dupe(u8, token2.value.name);
                            try element.value.property_access.append(name2);
                        },
                        else => {
                            self.lexer.unlex(&token2);
                            break :loop;
                        },
                    }
                }
            },
            .open_brace => {
                element.value = .{
                    .properties = std.ArrayList(*AstProperty).init(self.allocator),
                };
                loop: while (true) {
                    var token2 = Token.initEmpty();
                    const token_type2 = try self.lexer.lex(&token2);
                    switch (token_type2) {
                        .close_brace => {
                            break :loop;
                        },
                        else => {
                            self.lexer.unlex(&token2);
                            const next = try self.parseProperty();
                            try element.value.properties.append(next.?);
                        },
                    }
                }
            },
            .integer => {
                element.value = .{
                    .integer = token.value.integer,
                };
            },
            .string => {
                element.value = .{
                    .string = try self.allocator.dupe(u8, token.value.string),
                };
            },
            else => {
                self.lexer.unlex(&token);
            },
        }

        return element;
    }

    fn expectToken(self: *@This(), token_type: LexTokenType, token: ?*Token) anyerror!void {
        var emptyToken = Token.initEmpty();
        var out: *Token = if (token) |t| t else &emptyToken;
        _ = try self.lexer.lex(out);

        var tokens = std.ArrayList(LexTokenType).init(self.allocator);
        try tokens.append(token_type);

        try synassert(self.allocator, out.*, out.type == token_type, tokens);
        if (token == null) {
            out.tokenFinish();
        }
    }
};
