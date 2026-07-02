import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class JdbcSqlRunner {
    public static void main(String[] args) throws Exception {
        String driverJar = null;
        String driverClass = null;
        String jdbcUrl = null;
        String username = null;
        String password = null;
        String sql = null;

        for (int i = 0; i < args.length; ++i) {
            final String arg = args[i];
            if ("--driver-jar".equals(arg) && i + 1 < args.length) {
                driverJar = args[++i];
            } else if ("--driver-class".equals(arg) && i + 1 < args.length) {
                driverClass = args[++i];
            } else if ("--jdbc-url".equals(arg) && i + 1 < args.length) {
                jdbcUrl = args[++i];
            } else if ("--username".equals(arg) && i + 1 < args.length) {
                username = args[++i];
            } else if ("--password".equals(arg) && i + 1 < args.length) {
                password = args[++i];
            } else if ("--sql".equals(arg) && i + 1 < args.length) {
                sql = args[++i];
            }
        }

        if (driverClass == null || jdbcUrl == null || username == null || password == null || sql == null) {
            writeError("missing required arguments");
            return;
        }

        Class.forName(driverClass);
        try (Connection connection = DriverManager.getConnection(jdbcUrl, username, password);
             Statement statement = connection.createStatement()) {
            boolean hasResultSet = statement.execute(sql);
            List<Map<String, String>> lastRows = null;
            String message = "";

            do {
                if (hasResultSet) {
                    try (ResultSet resultSet = statement.getResultSet()) {
                        if (resultSet != null) {
                            lastRows = readRows(resultSet);
                        }
                    }
                } else {
                    final int updateCount = statement.getUpdateCount();
                    if (updateCount >= 0) {
                        message = "affected:" + updateCount;
                    }
                }
                hasResultSet = statement.getMoreResults();
            } while (hasResultSet || statement.getUpdateCount() != -1);

            if (lastRows != null) {
                writeJson(true, "", lastRows);
            } else {
                writeJson(true, message, new ArrayList<>());
            }
        } catch (Exception ex) {
            writeError(ex.getMessage() == null ? ex.toString() : ex.getMessage());
            return;
        }
    }

    private static List<Map<String, String>> readRows(ResultSet resultSet) throws Exception {
        final ResultSetMetaData meta = resultSet.getMetaData();
        final int columnCount = meta.getColumnCount();
        final List<Map<String, String>> rows = new ArrayList<>();
        while (resultSet.next()) {
            final Map<String, String> row = new LinkedHashMap<>();
            for (int column = 1; column <= columnCount; ++column) {
                final String label = meta.getColumnLabel(column);
                final Object value = resultSet.getObject(column);
                row.put(label, value == null ? "" : String.valueOf(value));
            }
            rows.add(row);
        }
        return rows;
    }

    private static void writeSuccess(ResultSet resultSet) throws Exception {
        writeJson(true, "", readRows(resultSet));
    }

    private static void writeError(String message) {
        writeJson(false, message == null ? "unknown error" : message, new ArrayList<>());
    }

    private static void writeJson(boolean ok, String message, List<Map<String, String>> rows) {
        final PrintWriter writer =
            new PrintWriter(new OutputStreamWriter(System.out, StandardCharsets.UTF_8), true);
        writer.print("{\"ok\":");
        writer.print(ok ? "true" : "false");
        writer.print(",\"message\":");
        writer.print(quoteJson(message));
        writer.print(",\"rows\":[");
        for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
            if (rowIndex > 0) {
                writer.print(',');
            }
            writer.print('{');
            final Map<String, String> row = rows.get(rowIndex);
            int columnIndex = 0;
            for (Map.Entry<String, String> entry : row.entrySet()) {
                if (columnIndex++ > 0) {
                    writer.print(',');
                }
                writer.print(quoteJson(entry.getKey()));
                writer.print(':');
                writer.print(quoteJson(entry.getValue()));
            }
            writer.print('}');
        }
        writer.print("]}");
        writer.flush();
    }

    private static String quoteJson(String value) {
        if (value == null) {
            return "\"\"";
        }
        return "\"" + value.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r")
            + "\"";
    }
}
