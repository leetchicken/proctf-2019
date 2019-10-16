# Drone racing

The "drone racing" service allows users to create maze-like levels and submit drone programs that will navigate a drone in a race from one corner of the maze to another.

There is HTTP REST API which supports the following requests:
* `POST /api/users` — register a new user
* `POST /api/users`/login — login

Other endpoints are only available for authenticated users:

* `POST /api/levels` — create a new level
* `GET /api/levels` — retrieve all levels
* `POST /api/programs` — submit a program for a level
* `GET /api/programs` — retrieve the list of your programs for a level
* `POST /api/runs` — run one of your programs with some params
* `GET /api/runs` — retrieve all runs for some level, see the scoreboard of competitors

Flags from the checksystem are embedded into drone programs: once they finish the level, they print a flag to stdout.

There are two known vulnerabilities: one of them allows to win any competition for any level, the other one allows to steal flags.

## Calling setMaze() is available from within the program

You can call the `setMaze(String)` method from your drone program. This method is used by the code runner on the server side to set the maze for the current program. If you run `setMaze(".")`, you will win with 0 moves.

This vulnerability doesn't allow to steal flags.

## RCE by overriding the base class of a generated class file

The second vulnerability is more interesting.

Programs passed to the server are compiled into JVM bytecode. In the first pass the program is compiled into commands for the abstract stack machine. In the second pass the abstract stack machine program is compiled into to JVM code via ObjectWeb ASM library. While compiling into JVM, the name of the resulting class is a placeholder containing "ae/hitb/proctf/drone_racing/DroneRacingProgram".

JVM uses a pool of constants — a special section in the .class file that contains all the constants used in the program. Constants are typed and follow each other. One of the types is UTF-8 string. The constants in the pool are unique, so despite the fact that the class name is used more than once in the bytecode, the corresponding string is stored only once, somewhere in the constant pool.

Another interesting type is ClassName. ClassName constants don't hold the actual string but rather a reference (index in the constant pool) to a UTF-8 string. So, when the class name is processed, it first takes the reference and follows it to retrieve the actual string.

Once the class file has been generated, the service replaces a placeholder name in the constant pool with the actual title of the program. The service checks that the title of the program does not exceed 46 characters - the length of the placeholder. If the program title is shorter, it is padded to 46 characters.

BUT. The class name is checked as a string, but is then stored in the .class file as a UTF-8 encoded byte sequence. Therefore we can overflow the constant if we use non-ASCII chars. We can not use 4-byte UTF-8 characters (because .class files have a *peculiar* UTF-8 implementation), but we can use 3-byte UTF-8 characters and overflow up to 2 * 46 bytes!

The class name constant in the constant pool is followed by the base class name, so we can overflow the base class name to inherit some useful methods! But we have to preserve the base class name length, otherwise all constant pool references will become invalid and the program won't launch. Luckily, the actual layout of constant pool is as follows:
```
...
"class name",
class name reference,
"base class name",
base class name reference,
...
```

So what we can do is we can store the desired base class elsewhere in the constant pool, then overwrite the original base class name with junk bytes, and then overwrite the base class name _reference_ to make it point to our desired base class name.

Okay, now we need to find a class in our assembly which will help us achieve running arbitrary code. So we extract all class files from jars and grep them for "Runtime.getRuntime().exec()" calls. There are 3 methods in 3 classes with this call:

1. `org.eclipse.jetty.servlets.CGI.exec()` — not suitable, because it has complex arguments (`(File command, String pathInfo, HttpServletRequest req, HttpServletResponse res)`), which we can't pass from our program
2. `org.h2.util.Profiler.exec(String... args)` — this method looks good, but it's private, and we can't call private methods from the child class.
3. `org.h2.tools.Server.openBrowser(String url)` — yes! It's a public static method with one String argument. We can call it from our class. But... It does not execute arbitrary commands, it just opens a browser.

Let's refer to the source code of this last method: https://github.com/h2database/h2database/blob/master/h2/src/main/org/h2/tools/Server.java#L649
It has complex logic for defining which program constitutes a "browser". We can see that if browser name contains `"%url"`, that browser name is split by a comma, `%url` is replaced with `url` (the method argument), and the resulting command is executed. Okay, now how we can define a browser name?

Let's see at
`String browser = Utils.getProperty(SysProperties.H2_BROWSER, null);`

`Utils.getProperty` is a method from https://github.com/h2database/h2database/blob/master/h2/src/main/org/h2/util/Utils.java#L722. It looks into Java system properties. But wait a second! We can define these properties with run params when we launch our drone program!

To recap, this is the outline of the exploit:
1. Register the user and log into the service;
2. Upload a drone program which is constructed like this:
	a. title has non-ASCII chars to overflow the class name and change the base class to "h2/tools/Server";
	b. `setMaze(String)` function is defined in the program, otherwise running the code will fail before start;
	c. there's a call to `openBrowser("programs/")` somewhere in the program;
3. Run the drone program with params `"h2.browser": "tar,czf,static/classes.tar.gz,%url"`

Once run, this program will archive all the class files uploaded by all participants and put the archive to static/classes.tar.gz, which is available via static file downloading.
