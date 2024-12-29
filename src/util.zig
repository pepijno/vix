const std = @import("std");

pub var sources: std.ArrayList([]const u8) = undefined;

pub fn vixPanic(comptime format: []const u8, src: std.builtin.SourceLocation, args: anytype) noreturn {
    const stderr = std.io.getStdErr().writer();
    _ = stderr.print("{s}:{}:{} ", .{ src.file, src.line, src.column }) catch unreachable;
    _ = stderr.print(format, args) catch unreachable;
    _ = stderr.print("\n", .{}) catch unreachable;
    std.process.abort();
}

pub fn vixUnreachable(src: std.builtin.SourceLocation) noreturn {
    vixPanic("unreachable", src, .{});
}

pub const Location = struct {
    file: u32,
    line_number: u32,
    column_number: u32,

    pub const ErrorLineError = (std.fmt.AllocPrintError || std.fs.File.OpenError || std.fs.File.WriteError);

    pub fn errorLine(self: @This(), allocator: std.mem.Allocator) ErrorLineError!void {
        const path = sources.items[self.file];
        const file = try std.fs.cwd().openFile(path, .{});
        defer file.close();

        const reader = file.reader();

        var line = std.ArrayList(u8).init(allocator);
        defer line.deinit();
        const writer = line.writer();

        const stderr = std.io.getStdErr().writer();
        for (0..self.line_number) |_| {
            line.clearRetainingCapacity();
            reader.streamUntilDelimiter(writer, '\n', null) catch return;
        }

        try stderr.print("\n", .{});
        const line_number = try std.fmt.allocPrint(allocator, "{}", .{self.line_number});
        try stderr.print("{s} |\t{s}{s}", .{
            line_number,
            line.items,
            if (line.getLast() == '\n') "" else "\n",
        });
        try stderr.print("{[a]s: >[length_line_number]} |\n{[a]s: >[length_column]}", .{
            .a = "",
            .length_line_number = line_number.len,
            .length_column = self.column_number - 1,
        });
    }
};
