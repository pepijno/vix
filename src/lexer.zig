const std = @import("std");

const util = @import("util.zig");
const Location = util.Location;

pub const LexTokenType = enum {
    none,
    name,
    assign,
    dot,
    dot_dot_dot,
    comma,
    character,
    string,
    open_brace,
    close_brace,
    open_paren,
    close_paren,
    greater_than,
    semicolon,
    integer,
    eof,
};

pub const Token = struct {
    type: LexTokenType,
    location: Location,
    value: union {
        name: []const u8,
        character: u8,
        string: []const u8,
        integer: u64,
    },

    pub fn initEmpty() @This() {
        return .{
            .type = .none,
            .location = std.mem.zeroes(Location),
            .value = .{
                .integer = 0,
            },
        };
    }

    pub fn tokenFinish(self: *@This()) void {
        self.type = .none;
        self.value = .{ .integer = 0 };
        self.location = std.mem.zeroInit(Location, .{});
    }
};

pub const Lexer = struct {
    allocator: std.mem.Allocator,
    source_iterator: std.unicode.Utf8Iterator,
    buffer: std.ArrayList(u8),
    c: [2]?u21,
    un: Token,
    location: Location,

    pub fn init(allocator: std.mem.Allocator, source: []const u8, file_id: u32) !@This() {
        const view = try std.unicode.Utf8View.init(source);
        return .{
            .allocator = allocator,
            .source_iterator = view.iterator(),
            .buffer = std.ArrayList(u8).init(allocator),
            .un = .{
                .location = std.mem.zeroInit(Location, .{}),
                .type = .none,
                .value = .{
                    .integer = 0,
                },
            },
            .location = .{
                .line_number = 1,
                .column_number = 1,
                .file = file_id,
            },
            .c = std.mem.zeroes([2]?u21),
        };
    }

    pub fn unlex(self: *@This(), out: *Token) void {
        std.debug.assert(self.un.type == .none);
        self.un = out.*;
    }

    pub fn lex(self: *@This(), out: *Token) anyerror!LexTokenType {
        if (self.un.type != .none) {
            out.* = self.un;
            self.un.type = .none;
            return out.type;
        }

        var cp = try self.wgetc(&out.location);
        if (cp == null) {
            out.type = .eof;
            return out.type;
        }
        const character = cp.?;
        if (character <= 0x7F and std.ascii.isDigit(@intCast(character))) {
            self.push(character, false);
            try self.lexNumber(out);
            return .integer;
        }
        if (character <= 0x7F and std.ascii.isAlphabetic(@intCast(character))) {
            self.push(character, false);
            try self.lexName(out);
            return .name;
        }

        switch (character) {
            '<' => {
                self.push(character, false);
                try self.lexString(out);
                return .string;
            },
            '.' => {
                cp = try self.next(null, false);
                std.debug.assert(cp != null);
                switch (cp.?) {
                    '.' => {
                        cp = try self.next(null, false);
                        std.debug.assert(cp != null);
                        switch (cp.?) {
                            '.' => out.type = .dot_dot_dot,
                            else => try self.printError("Unknown sequence '..'", .{}),
                        }
                    },
                    else => {
                        self.push(cp.?, false);
                        out.type = .dot;
                    },
                }
            },
            '=' => out.type = .assign,
            ',' => out.type = .comma,
            '{' => out.type = .open_brace,
            '}' => out.type = .close_brace,
            '(' => out.type = .open_paren,
            ')' => out.type = .close_paren,
            '>' => out.type = .greater_than,
            ';' => out.type = .semicolon,
            else => try self.printError("Unexpected character '{}'", .{character}),
        }
        return out.type;
    }

    fn clearBuffer(self: *@This()) void {
        self.buffer.clearRetainingCapacity();
    }

    fn printError(self: *@This(), comptime format: []const u8, args: anytype) anyerror!noreturn {
        const stderr = std.io.getStdErr().writer();
        _ = try stderr.print("{s}:{}:{}: syntax error: ", .{ "vix", self.location.line_number, self.location.column_number });
        _ = try stderr.print(format, args);
        _ = try stderr.print("\n", .{});
        try self.location.errorLine(self.allocator);
        std.process.exit(2);
    }

    fn updateLineNumber(location: *Location, character: ?u21) void {
        if (character == null) {
            return;
        }
        if (character.? == '\n') {
            location.line_number += 1;
            location.column_number = 1;
        } else if (character.? == '\t') {
            location.column_number += 4;
        } else {
            location.line_number += 1;
        }
    }

    fn next(self: *@This(), location: ?*Location, is_buffer: bool) anyerror!?u21 {
        var character: ?u21 = null;
        if (self.c[0] != null) {
            character = self.c[0];
            self.c[0] = self.c[1];
            self.c[1] = null;
        } else {
            character = self.source_iterator.nextCodepoint();
            if (character == null) {
                return null;
                // self.printError("Invalid UTF-8 character", .{});
            }
            updateLineNumber(&self.location, character);
        }

        if (location) |loc| {
            loc.* = self.location;
            var i: usize = 0;
            while (i < 2 and self.c[i] != null) : (i += 1) {
                updateLineNumber(&self.location, self.c[i]);
            }
        }

        if (character == null or !is_buffer) {
            return character;
        }
        var encoded_result = [_]u8{0} ** 4;
        const length = try std.unicode.utf8Encode(character.?, &encoded_result);
        for (encoded_result[0..length]) |char| {
            try self.buffer.append(char);
        }
        return character;
    }

    fn wgetc(self: *@This(), location: ?*Location) !?u21 {
        var cp: ?u21 = null;
        while (true) {
            cp = try self.next(location, false);
            if (cp == null or !std.ascii.isWhitespace(@intCast(cp.?))) {
                break;
            }
        }
        return cp;
    }

    fn consume(self: *@This(), n: usize) void {
        var i: usize = 0;
        while (i < n) : (i += 1) {
            while (self.buffer.items.len != 0) {
                const item = self.buffer.pop();
                if ((item & 0xC0) != 0x80) {
                    break;
                }
            }
        }
    }

    fn push(self: *@This(), character: u21, is_buffer: bool) void {
        std.debug.assert(self.c[1] == null);
        self.c[1] = self.c[0];
        self.c[0] = character;
        if (is_buffer) {
            self.consume(1);
        }
    }

    fn lexNumber(self: *@This(), out: *Token) !void {
        var cp = try self.next(&out.location, true);
        std.debug.assert(cp != null);
        var character = cp.?;
        std.debug.assert(character <= 0x7F and std.ascii.isDigit(@intCast(character)));

        while (true) {
            if (cp == null) {
                break;
            }

            character = cp.?;
            if (!std.ascii.isDigit(@intCast(character))) {
                break;
            }
            cp = try self.next(null, true);
        }
        self.push(character, true);

        out.value = .{
            .integer = try std.fmt.parseInt(u64, self.buffer.items, 10),
        };
        out.type = .integer;
        self.clearBuffer();
    }

    fn lexName(self: *@This(), out: *Token) anyerror!void {
        var cp = try self.next(&out.location, true);
        std.debug.assert(cp != null);
        var character = cp.?;
        std.debug.assert(character <= 0x7F and std.ascii.isAlphabetic(@intCast(character)));
        while (cp != null) {
            cp = try self.next(&out.location, true);
            if (cp) |codepoint| {
                character = codepoint;
                if (character > 0x7F or (!std.ascii.isAlphanumeric(@intCast(character)) and character != '_')) {
                    self.push(character, true);
                    break;
                }
            }
        }

        out.type = .name;
        out.value = .{
            .name = try self.allocator.dupe(u8, self.buffer.items),
        };
        self.clearBuffer();
    }

    fn lexRune(self: *@This()) anyerror![]const u8 {
        const cp = try self.next(null, false);
        std.debug.assert(cp != null);
        var character = cp.?;

        switch (character) {
            '\\' => {
                character = (try self.next(null, false)).?;
                switch (character) {
                    // '0', 'a' => {
                    //     return "\\0";
                    // },
                    // 'b' => {
                    //     return .{
                    //         .characters = [_]u8{ '\b', 0, 0, 0 },
                    //         .length = 1,
                    //     };
                    // },
                    'r' => {
                        return "\r";
                    },
                    't' => {
                        return "\t";
                    },
                    'n' => {
                        return "\n";
                    },
                    // 'f' => {
                    //     return .{
                    //         .characters = [_]u8{ '\f', 0, 0, 0 },
                    //         .length = 1,
                    //     };
                    // },
                    // 'v' => {
                    //     return .{
                    //         .characters = [_]u8{ '\v', 0, 0, 0 },
                    //         .length = 1,
                    //     };
                    // },
                    '\\' => {
                        return "\\";
                    },
                    else => try self.printError("Invalid escape '\\{}'", .{character}),
                }
                util.vixUnreachable(@src());
            },
            else => {
                var encoded_result = [_]u8{0} ** 4;
                const length = try std.unicode.utf8Encode(character, &encoded_result);
                return encoded_result[0..length];
            }, // TODO
        }
    }

    fn lexString(self: *@This(), out: *Token) anyerror!void {
        var cp = try self.next(&out.location, false);
        std.debug.assert(cp != null);
        var character = cp.?;

        switch (character) {
            '<' => {
                var count: usize = 1;
                while (true) {
                    cp = try self.next(null, false);
                    if (cp == null) {
                        try self.printError("Unexpected end of file", .{});
                    }
                    character = cp.?;
                    if (character == '<') {
                        count += 1;
                    } else if (character == '>' and count == 1) {
                        break;
                    } else if (character == '>') {
                        count -= 1;
                    }
                    self.push(character, false);
                    const utf8Char = try self.lexRune();
                    for (utf8Char) |char| {
                        try self.buffer.append(char);
                    }
                }
                out.type = .string;
                out.value = .{
                    .string = try self.allocator.dupe(u8, self.buffer.items),
                };
                self.clearBuffer();
                return;
            },
            else => util.vixUnreachable(@src()),
        }
    }
};
