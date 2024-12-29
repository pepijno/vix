const std = @import("std");

const AstProperty = @import("ast.zig").AstProperty;
const TypecheckEnv = @import("types.zig").TypecheckEnv;

const Edge = struct {
    from: usize,
    to: usize,
};

const Group = struct {
    members: std.AutoHashMap(usize, void),
};

const GroupData = struct {
    functions: std.AutoHashMap(usize, void),
    adjacency_list: std.AutoHashMap(usize, void),
    indegree: usize,

    fn init(allocator: std.mem.Allocator) !*GroupData {
        const group_data = try allocator.create(GroupData);
        group_data.* = .{
            .functions = std.AutoHashMap(usize, void).init(allocator),
            .adjacency_list = std.AutoHashMap(usize, void).init(allocator),
            .indegree = 0,
        };
        return group_data;
    }
};

pub const ObjectGraph = struct {
    allocator: std.mem.Allocator,
    adjacency_list: std.AutoHashMap(usize, std.AutoHashMap(usize, void)),
    edges: std.AutoHashMap(Edge, void),

    pub fn init(allocator: std.mem.Allocator) @This() {
        return .{
            .allocator = allocator,
            .adjacency_list = std.AutoHashMap(usize, std.AutoHashMap(usize, void)).init(allocator),
            .edges = std.AutoHashMap(Edge, void).init(allocator),
        };
    }

    pub fn addFunction(self: *@This(), from: usize) !*std.AutoHashMap(usize, void) {
        const search_result = self.adjacency_list.getPtr(from);
        if (search_result) |value| {
            return value;
        } else {
            const new_set = std.AutoHashMap(usize, void).init(self.allocator);
            try self.adjacency_list.put(from, new_set);
            return self.adjacency_list.getPtr(from).?;
        }
    }

    pub fn initProperties(self: *@This(), properties: std.ArrayList(*AstProperty), env: *TypecheckEnv) !void {
        for (properties.items) |prop| {
            try env.addName(prop.name, prop.id);
        }

        for (properties.items) |prop| {
            _ = try self.addFunction(prop.id);
            switch (prop.value.value) {
                .properties => |props| {
                    for (props.items) |p| {
                        try self.addEdge(p.id, prop.id);
                    }
                    var new_env = TypecheckEnv.init(env.allocator, env);
                    try self.initProperties(prop.value.value.properties, &new_env);
                },
                .property_access => |ids| {
                    const name = ids.items[0];
                    const id = env.find(name);
                    try self.addEdge(id, prop.id);
                },
                else => {},
            }
        }
    }

    pub fn addEdge(self: *@This(), from: usize, to: usize) !void {
        var set = try self.addFunction(from);
        try set.put(to, {});
        try self.edges.put(.{
            .from = from,
            .to = to,
        }, {});
    }

    pub fn computeOrder(self: *@This()) !std.ArrayList(*Group) {
        const transitive_edges = try self.computeTransitiveEdges();
        var group_ids = std.AutoHashMap(usize, usize).init(self.allocator);
        var group_data_map = std.AutoHashMap(usize, *GroupData).init(self.allocator);
        try self.createGroups(transitive_edges, &group_ids, &group_data_map);
        try self.createEdges(&group_ids, &group_data_map);
        return try self.generateOrder(group_data_map);
    }

    fn computeTransitiveEdges(self: *@This()) !std.AutoHashMap(Edge, void) {
        var transitive_edges = try self.edges.clone();
        var it_connector = self.adjacency_list.keyIterator();
        while (it_connector.next()) |connector| {
            var it_from = self.adjacency_list.keyIterator();
            while (it_from.next()) |from| {
                const to_connector: Edge = .{
                    .from = from.*,
                    .to = connector.*,
                };
                var it_to = self.adjacency_list.keyIterator();
                while (it_to.next()) |to| {
                    const full_jump: Edge = .{
                        .from = from.*,
                        .to = to.*,
                    };
                    if (transitive_edges.contains(full_jump)) {
                        continue;
                    }
                    const from_connector: Edge = .{
                        .from = connector.*,
                        .to = to.*,
                    };
                    if (transitive_edges.contains(to_connector) and transitive_edges.contains(from_connector)) {
                        try transitive_edges.put(full_jump, {});
                    }
                }
            }
        }
        return transitive_edges;
    }

    fn createGroups(self: @This(), transitive_edges: std.AutoHashMap(Edge, void), group_ids: *std.AutoHashMap(usize, usize), group_data_map: *std.AutoHashMap(usize, *GroupData)) !void {
        var id_counter: usize = 0;
        var it_vertex = self.adjacency_list.keyIterator();
        while (it_vertex.next()) |vertex| {
            if (group_ids.contains(vertex.*)) {
                continue;
            }

            var new_group = try GroupData.init(self.allocator);
            try new_group.functions.put(vertex.*, {});
            try group_data_map.put(id_counter, new_group);
            try group_ids.put(vertex.*, id_counter);
            var it_other_vertex = self.adjacency_list.keyIterator();
            while (it_other_vertex.next()) |other_vertex| {
                if (transitive_edges.contains(.{ .from = vertex.*, .to = other_vertex.* }) and transitive_edges.contains(.{ .from = other_vertex.*, .to = vertex.* })) {
                    try group_ids.put(other_vertex.*, id_counter);
                    try new_group.functions.put(other_vertex.*, {});
                }
            }

            id_counter += 1;
        }
    }

    fn createEdges(self: *@This(), group_ids: *std.AutoHashMap(usize, usize), group_data_map: *std.AutoHashMap(usize, *GroupData)) !void {
        var group_edges = std.AutoHashMap(Edge, void).init(self.allocator);
        var it_vertex = self.adjacency_list.iterator();
        while (it_vertex.next()) |vertex| {
            const vertex_id = try group_ids.getOrPut(vertex.key_ptr.*);
            const vertex_data = try group_data_map.getOrPut(vertex_id.value_ptr.*);
            var it_other_vertex = vertex.value_ptr.*.keyIterator();
            while (it_other_vertex.next()) |other_vertex| {
                const other_id = try group_ids.getOrPut(other_vertex.*);
                if (other_id.value_ptr.* == vertex_id.value_ptr.*) {
                    continue;
                }
                if (group_edges.contains(.{ .from = vertex_id.value_ptr.*, .to = other_id.value_ptr.* })) {
                    continue;
                }
                try group_edges.put(.{
                    .from = vertex_id.value_ptr.*,
                    .to = other_id.value_ptr.*,
                }, {});
                try vertex_data.value_ptr.*.adjacency_list.put(other_id.value_ptr.*, {});
                group_data_map.get(other_id.value_ptr.*).?.indegree += 1;
            }
        }
    }

    fn generateOrder(self: *@This(), group_data_map: std.AutoHashMap(usize, *GroupData)) !std.ArrayList(*Group) {
        var id_queue = std.fifo.LinearFifo(usize, .Dynamic).init(self.allocator);
        var output = std.ArrayList(*Group).init(self.allocator);

        var it = group_data_map.iterator();
        while (it.next()) |kv| {
            if (kv.value_ptr.*.indegree == 0) {
                try id_queue.writeItem(kv.key_ptr.*);
            }
        }

        while (id_queue.readItem()) |new_id| {
            const group_data = group_data_map.get(new_id).?;
            var output_group = try self.allocator.create(Group);
            output_group.members = try group_data.functions.clone();

            var group_it = group_data.adjacency_list.keyIterator();
            while (group_it.next()) |adjacent_group| {
                const data = group_data_map.get(adjacent_group.*).?;
                data.indegree -= 1;
                if (data.indegree == 0) {
                    try id_queue.writeItem(adjacent_group.*);
                }
            }

            try output.append(output_group);
        }

        return output;
    }
};
