const std = @import("std");

const Type = @import("types.zig").Type;

pub const AstElementType = enum {
    integer,
    string,
    properties,
    property_access,
};

pub const AstProperty = struct {
    id: usize,
    name: []const u8,
    value: *AstElement,
    prop_type: *Type,
};

pub const AstElement = struct {
    value: union(AstElementType) {
        integer: u64,
        string: []const u8,
        properties: std.ArrayList(*AstProperty),
        property_access: std.ArrayList([]const u8),
    },
};
