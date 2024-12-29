const std = @import("std");

const graph = @import("graph.zig");
const lex = @import("lexer.zig");
const Lexer = lex.Lexer;
const Token = lex.Token;
const Location = lex.Location;
const types = @import("types.zig");
const TypecheckEnv = types.TypecheckEnv;
const TypeEnv = types.TypeEnv;
const TypeContext = types.TypeContext;
const util = @import("util.zig");
const printing = @import("printing.zig");
const Parser = @import("parser.zig").Parser;

pub fn main() !void {
    const stderr_file = std.io.getStdErr().writer();
    var stderr_bw = std.io.bufferedWriter(stderr_file);
    const stderr = stderr_bw.writer();

    const stdout_file = std.io.getStdOut().writer();
    var stdout_bw = std.io.bufferedWriter(stdout_file);
    const stdout = stdout_bw.writer();

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        try stderr.print("Please provide a *.vix file to compile\n", .{});
        try stderr_bw.flush();
        std.process.exit(1);
    }

    const file = try std.fs.cwd().openFile(args[1], .{});
    defer file.close();
    const content = try file.readToEndAlloc(allocator, std.math.maxInt(usize));
    var lexer = try Lexer.init(allocator, content, 0);
    util.sources = std.ArrayList([]const u8).init(allocator);
    try util.sources.append(args[1]);

    var parser = Parser.init(allocator, &lexer);
    const root = try parser.parse();
    try printing.printElement(root.*, 0, stdout);

    try stdout_bw.flush();

    var obj_graph = graph.ObjectGraph.init(allocator);
    defer obj_graph.deinit();

    var typecheck_env = TypecheckEnv.init(allocator, null);
    try obj_graph.initProperties(root.value.properties, &typecheck_env);

    const groups = try obj_graph.computeOrder();

    for (groups.items) |group| {
        var it = group.members.keyIterator();
        while (it.next()) |member| {
            const result = parser.properties.get(member.*);
            std.debug.assert(result != null);
            try stdout.print("{s}, ", .{result.?.name});
        }
        try stdout.print("\n", .{});
    }

    var env = TypeEnv.init(allocator, null);
    var context = TypeContext.init(allocator);
    for (groups.items) |group| {
        var it = group.members.keyIterator();
        while (it.next()) |member| {
            const result = parser.properties.get(member.*);
            std.debug.assert(result != null);
            const prop = result.?;
            switch (prop.value.value) {
                .properties => {
                    try context.typecheckPropertiesFirst(&env, prop.name, prop.value.*);
                    try context.typecheckPropertiesSecond(&env, prop.value.*);
                },
                else => {
                    const t = try context.typecheck(&env, prop.value.*);
                    try env.bind(prop.name, t.?);
                },
            }
        }
    }

    var names_it = env.names.valueIterator();
    while (names_it.next()) |t| {
        try printing.printType(context, t.*.*, 0, stdout);
    }

    try stdout_bw.flush();
    try stderr_bw.flush();
}
