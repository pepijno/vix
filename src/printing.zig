const std = @import("std");

const ast = @import("ast.zig");
const AstProperty = ast.AstProperty;
const AstElement = ast.AstElement;
const Token = @import("lexer.zig").Token;
const types = @import("types.zig");
const Type = types.Type;
const TypeContext = types.TypeContext;

pub fn printToken(token: Token, writer: anytype) !void {
    try writer.print("{s}", .{@tagName(token.type)});
    switch (token.type) {
        .name => try writer.print(" {s}", .{token.value.name}),
        .character => try writer.print(" {c}", .{token.value.character}),
        .string => try writer.print(" {s}", .{token.value.string}),
        .integer => try writer.print(" {}", .{token.value.integer}),
        else => {},
    }
    try writer.print("\n", .{});
}

fn printIndent(indent: usize, writer: anytype) std.fs.File.WriteError!void {
    try writer.print("{[s]s: >[size]}", .{ .s = "", .size = 2 * indent });
}

fn printProperty(property: AstProperty, indent: usize, writer: anytype) std.fs.File.WriteError!void {
    try printIndent(indent, writer);
    try writer.print("PROPERTY {}: {s}\n", .{ property.id, property.name });
    try printElement(property.value.*, indent + 1, writer);
}

pub fn printElement(element: AstElement, indent: usize, writer: anytype) std.fs.File.WriteError!void {
    switch (element.value) {
        .integer => |int| {
            try printIndent(indent, writer);
            try writer.print("INTEGER: {}\n", .{int});
        },
        .string => |str| {
            try printIndent(indent, writer);
            try writer.print("STRING: {s}\n", .{str});
        },
        .properties => |props| {
            try printIndent(indent, writer);
            try writer.print("PROPERTIES: \n", .{});
            for (props.items) |prop| {
                try printProperty(prop.*, indent + 1, writer);
            }
        },
        .property_access => |ids| {
            try printIndent(indent, writer);
            try writer.print("PROPERTY ACCESS: \n", .{});
            try printIndent(indent + 1, writer);
            for (ids.items) |id| {
                try writer.print("{s}, ", .{id});
            }
            try writer.print("\n", .{});
        },
    }
}

pub fn printType(context: TypeContext, t: Type, indent: usize, writer: anytype) std.fs.File.WriteError!void {
    switch (t.value) {
        .base => |b| {
            try printIndent(indent, writer);
            try writer.print("{s}\n", .{b.name});
        },
        .variable => |v| {
            const tt = context.types.get(v.name);
            if (tt) |var_type| {
                try printType(context, var_type.*, indent, writer);
            } else {
                try printIndent(indent, writer);
                try writer.print("VAR {s}\n", .{v.name});
            }
        },
        .arrow => |a| {
            try printIndent(indent, writer);
            try printType(context, a.left.*, indent, writer);
            try writer.print(" -> (", .{});
            try printType(context, a.right.*, indent, writer);
            try writer.print(")\n", .{});
        },
        .properties => |props| {
            try printIndent(indent, writer);
            try writer.print("{{\n", .{});
            for (props.items) |prop| {
                try printIndent(indent + 1, writer);
                try writer.print("{s}:\n", .{prop.name});
                try printType(context, prop.prop_type.*, indent + 2, writer);
            }
            try printIndent(indent, writer);
            try writer.print("}}\n", .{});
        },
    }
}
