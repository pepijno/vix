const std = @import("std");

const ast = @import("ast.zig");
const AstElement = ast.AstElement;
const AstElementType = ast.AstElementType;
const graph = @import("graph.zig");
const util = @import("util.zig");
const ObjectGraph = graph.ObjectGraph;

pub const TypecheckEnv = struct {
    allocator: std.mem.Allocator,
    parent: ?*const TypecheckEnv,
    names: std.StringHashMap(usize),

    pub fn init(allocator: std.mem.Allocator, parent: ?*const TypecheckEnv) @This() {
        return .{
            .allocator = allocator,
            .parent = parent,
            .names = std.StringHashMap(usize).init(allocator),
        };
    }

    pub fn addName(self: *@This(), name: []const u8, id: usize) std.mem.Allocator.Error!void {
        try self.names.put(name, id);
    }

    pub fn find(self: @This(), name: []const u8) usize {
        const result = self.names.get(name);
        if (result) |id| {
            return id;
        } else {
            if (self.parent) |parent| {
                return parent.find(name);
            } else {
                util.vixUnreachable(@src());
            }
        }
    }
};

pub const TypeEnv = struct {
    allocator: std.mem.Allocator,
    names: std.StringHashMap(*Type),
    parent: ?*const @This(),

    pub fn init(allocator: std.mem.Allocator, parent: ?*const @This()) @This() {
        return .{
            .allocator = allocator,
            .names = std.StringHashMap(*Type).init(allocator),
            .parent = parent,
        };
    }

    pub fn lookup(self: @This(), name: []const u8) ?*Type {
        const id = self.names.get(name);
        if (id) |i| {
            return i;
        } else if (self.parent) |p| {
            return p.lookup(name);
        } else {
            return null;
        }
    }

    pub fn bind(self: *@This(), name: []const u8, t: *Type) !void {
        try self.names.put(name, t);
    }
};

pub const TypeType = enum {
    variable,
    base,
    arrow,
    properties,
};

const TypeVariable = struct {
    name: []const u8,
};

const TypeBase = struct {
    name: []const u8,
};

const TypeArrow = struct {
    left: *Type,
    right: *Type,
};

const TypeProperty = struct {
    name: []const u8,
    prop_type: *Type,
};

pub const Type = struct {
    value: union(TypeType) {
        variable: TypeVariable,
        base: TypeBase,
        arrow: TypeArrow,
        properties: std.ArrayList(*TypeProperty),
    },
};

pub const TypeContext = struct {
    last_id: usize,
    allocator: std.mem.Allocator,
    types: std.StringHashMap(*Type),

    pub fn init(allocator: std.mem.Allocator) @This() {
        return .{
            .last_id = 1,
            .allocator = allocator,
            .types = std.StringHashMap(*Type).init(allocator),
        };
    }

    pub fn typecheck(self: *@This(), env: *TypeEnv, element: AstElement) !?*Type {
        switch (element.value) {
            .integer => {
                var t = try self.allocator.create(Type);
                t.value = .{
                    .base = .{
                        .name = "Int",
                    },
                };
                return t;
            },
            .string => {
                var t = try self.allocator.create(Type);
                t.value = .{
                    .base = .{
                        .name = "Str",
                    },
                };
                return t;
            },
            .properties => {
                return null;
            },
            .property_access => |names| {
                var t: ?*Type = null;
                for (names.items) |name| {
                    if (t == null) {
                        t = env.lookup(name);
                    } else {
                        for (t.?.value.properties.items) |prop| {
                            if (std.mem.eql(u8, prop.name, name)) {
                                t = prop.prop_type;
                                break;
                            }
                        }
                        if (@as(TypeType, t.?.value) == .variable) {
                            const res = self.types.get(t.?.value.variable.name);
                            std.debug.assert(res != null);
                            t = res.?;
                        }
                    }
                }
                return t;
            },
        }
        util.vixUnreachable(@src());
    }

    pub fn typecheckPropertiesFirst(self: *@This(), env: *TypeEnv, name: []const u8, element: AstElement) !void {
        std.debug.assert(@as(AstElementType, element.value) == .properties);
        var t = try self.allocator.create(Type);
        t.value = .{
            .properties = std.ArrayList(*TypeProperty).init(self.allocator),
        };

        for (element.value.properties.items) |prop| {
            const prop_type = try self.createTypeVariable();
            var type_prop = try self.allocator.create(TypeProperty);
            type_prop.prop_type = prop_type;
            type_prop.name = try self.allocator.dupe(u8, prop.name);
            try t.value.properties.append(type_prop);
            prop.prop_type = prop_type;
        }

        try env.bind(name, t);
    }

    pub fn typecheckPropertiesSecond(self: *@This(), env: *TypeEnv, element: AstElement) !void {
        std.debug.assert(@as(AstElementType, element.value) == .properties);
        var new_env = TypeEnv.init(env.allocator, env);

        for (element.value.properties.items) |prop| {
            var typechecked_type = try self.typecheck(env, prop.value.*);
            if (@as(AstElementType, prop.value.value) == .properties) {
                typechecked_type = env.lookup(prop.name);
            }
            try self.unify(prop.prop_type, typechecked_type.?);
            try new_env.bind(prop.name, prop.prop_type);
        }
    }

    fn createTypeVariable(self: *@This()) !*Type {
        const name = try self.newTypeName();
        var t = try self.allocator.create(Type);

        t.value = .{
            .variable = .{
                .name = name,
            },
        };

        return t;
    }

    fn newTypeName(self: *@This()) ![]const u8 {
        const name = try std.fmt.allocPrint(self.allocator, "{}", .{self.last_id});
        self.last_id += 1;
        return name;
    }

    fn unify(self: *@This(), left: *Type, right: *Type) !void {
        var lvar: ?*Type = null;
        var rvar: ?*Type = null;

        const l = try self.resolve(left, &lvar);
        const r = try self.resolve(right, &rvar);

        if (lvar) |lv| {
            try self.bind(lv.value.variable.name, r);
            return;
        } else if (rvar) |rv| {
            try self.bind(rv.value.variable.name, l);
            return;
        } else if (@as(TypeType, l.value) == .arrow and @as(TypeType, r.value) == .arrow) {
            try self.unify(l.value.arrow.left, r.value.arrow.left);
            try self.unify(l.value.arrow.right, r.value.arrow.right);
            return;
        } else if (@as(TypeType, l.value) == .base and @as(TypeType, r.value) == .base) {
            if (std.mem.eql(u8, l.value.base.name, r.value.base.name)) {
                return;
            }
        }
        util.vixUnreachable(@src());
    }

    fn bind(self: *@This(), name: []const u8, t: *Type) !void {
        if (@as(TypeType, t.value) == .variable and std.mem.eql(u8, t.value.variable.name, name)) {
            return;
        }
        try self.types.put(name, t);
    }

    fn resolve(self: *@This(), t: *Type, type_var: *?*Type) !*Type {
        var tt = t;
        type_var.* = null;
        while (@as(TypeType, tt.value) == .variable) {
            const it = self.types.get(t.value.variable.name);
            if (it == null) {
                type_var.* = tt;
                break;
            } else {
                tt = it.?;
            }
        }

        return tt;
    }
};
