# Unix Restricted-I/O Shell & Descriptor Redirection System

A C-based systems-programming project that implements a custom command-line interface (CLI) shell for sandboxed file and operation logging. The project is designed around a unique architectural constraint: **performing all file reads and writes without utilizing direct `read()` or `write()` system calls**. Instead, it dynamically intercepts, duplicates, and redirects standard input (`stdin`, FD 0) and standard output (`stdout`, FD 1) streams using Unix descriptor manipulation APIs (`dup`/`dup2`).

---

## 🚀 Key Highlights & Systems Concepts

*   **Dynamic Descriptor Redirection (`dup`/`dup2`)**: Re-maps `stdin` and `stdout` on the fly. Dynamically shifts standard input/output streams between files and the console, executing formatting I/O (`printf`/`scanf`) to perform simulated file access.
*   **Dynamic Circular Buffer Implementation**: Implements a space-efficient circular buffer using nested memory pointers (`char **`) for `LAST n` and `LOG n` operations. This limits memory footprint to $O(n)$ where $n$ is the requested line limit, preventing unbounded buffer memory usage.
*   **Process Isolation (Sandboxing)**: Retrieves the running process ID (`getpid`) to dynamically create and bind execution context within a private, isolated directory (`folder_<PID>`). Prevents race conditions and namespace collisions when running multiple instances of the utility concurrently.
*   **System Stream State Recovery**: Explicitly manages Unix file pointer offsets and clears stream error states using `clearerr(stdin)` and flushing (`fflush(stdout)`) to prevent buffer corruption or stale EOF markers during state redirection.

---

## 🛠️ Commands Supported

The shell supports a list of commands executed against a target file `content.txt` and an audit log file `logs.txt` inside the process-isolated sandbox:

| Command | Arguments | Behavior |
| :--- | :--- | :--- |
| **`INPUT`** | `[line of text]` | Appends a full line of text to `content.txt` via stdout redirection, and appends `"INPUT"` to `logs.txt`. |
| **`PRINT`** | *None* | Reads and displays the entire contents of `content.txt` to the terminal via stdin redirection, logging the action. |
| **`FIRST`** | `n` | Reads and outputs the first `n` lines of `content.txt`, logging the action. |
| **`LAST`** | `n` | Reads and outputs the last `n` lines of `content.txt` using a dynamic circular buffer, logging the action. |
| **`LOG`** | `n` | Displays the last `n` log records from `logs.txt` (this command itself is not logged to prevent recursive inflation). |
| **`STOP`** | *None* | Logs the exit action and safely terminates the shell. |

---

## 🏗️ Building and Running

Ensure you are working in a Unix-like environment (Linux, WSL, or macOS) with `gcc` and `make` installed.

### Compile
To compile the shell with strict warnings and symbols, run:
```bash
make
```

### Run
To start the shell:
```bash
make run
```

### Clean
To remove the compiled binary and delete any sandboxed run folders (`folder_*`):
```bash
make clean
```

---

## 📝 Example Shell Session

```text
$ make run
./restricted_io_shell
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): INPUT
INPUT -> Hello World from Restricted IO Shell
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): INPUT
INPUT -> Operating Systems and Networks
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): FIRST 1
Hello World from Restricted IO Shell
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): LAST 1
Operating Systems and Networks
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): LOG 3
INPUT
INPUT
FIRST 1
Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): STOP
```

