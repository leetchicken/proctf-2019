# Drone racing

Service "drone racing" allows users to create levels (some kind of mazes) and submit programs for drones which should pass the drone from one corner of maze to another one.

There is HTTP REST API which support following requests:
* POST /api/users — register a new user
* POST /api/users/login — login

Other endpoints are available only for authenticated user:

* POST /api/levels — create a new level
* GET /api/levels — receive all levels
* POST /api/programs — submit a program for some level
* GET /api/programs — receive list of your programs for some level
* POST /api/runs — run one of yours program with some params
* GET /api/runs — receive all runs for some level, see the scoreboard of competitors

Flags from checksystem are stored in programs: they pass the maze and print flag to output.

There are two known vulnerabilities: one of them allows to win any competition for any level, and another one allows to steal flags

## Calling setMaze() is available from the program

You can call method setMaze(String) from your program. This method is used by code runner on the server side to set maze's map for current program. If you will run setMaze("."), you will win with 0 moves.

This vulnerability doesn't allow to steal flags.

## Override the base class of generated class file, what leads to RCE

Second vulnerability is more interesting.

Program passed to the server is compiling to JVM bytecode. First of all it is compiling to abstract stack machine, and after it program for this machine is compiling to JVM code via ObjectWeb ASM library. While compiling, the name of class is always "ae/hitb/proctf/drone_racing/DroneRacingProgram". JVM uses constant pool — special section in .class file which contains all constants used in the program. So despite of this class name is used more then once in bytecode, this string is stored only one, somewhere in the constant pool. 

After generating the class file, this name ("ae/hitb/proctf/drone_racing/DroneRacingProgram") is replaced to program's title. Program's title can not be longer than length of "ae/hitb/proctf/drone_racing/DroneRacingProgram", 46 symbols. In case of smaller title, it is padded to 46 symbols.

After this manipulation class in .class file has name, passed as program's title.

BUT. Class name is stored in .class file as bytes sequence, not as chars sequence. So we can overflow this segment if we will use non-ASCII chars. We can not use 4-bytes UTF-8 symbols (because .class files has *strange* UTF-8 implementation), but we can use 3-bytes UTF-8 symbols and overflow 2 * 46 bytes! 

Next UTF-8 string in the constant pool is base class name. We need to use class with the same name's length, which is non-hackable. But we can do a trick: put needed base class name as string somewhere in the constant pool and overflow only link to the string.

Okey, we need to find a class in our assemble which can help us to achieve running arbitrary code. So we extract all class files from jars and grep it for "Runtime.getRuntime().exec()" calls. There are 3 methods in 3 classes with this call:

1. `org.eclipse.jetty.servlets.CGI.exec()` — not suitable, because has complex arguments (`(File command, String pathInfo, HttpServletRequest req, HttpServletResponse res)`), which we can't pass from our program
2. `org.h2.util.Profiler.exec(String... args)` — this method is looks good, but it's private, we can't call private method from child class
3. `org.h2.tools.Server.openBrowser(String url)` — yes! It's public static method with one String argument. We can call it from our class. But... It's just opens a browser, not execute any command. 

Let's see to source code of last method: https://github.com/h2database/h2database/blob/master/h2/src/main/org/h2/tools/Server.java#L649
It has complex logic for defining which program is a "browser". We can see that if browser name contains "%url", that browser name is splitted by comma, %url is replaced to `url` (passed argument), and resulted command is executed. Okey, but how we can define a browser name?

Let's see at
`String browser = Utils.getProperty(SysProperties.H2_BROWSER, null);`

`Utils.getProperty` is a method from https://github.com/h2database/h2database/blob/master/h2/src/main/org/h2/util/Utils.java#L722. It looks to the Java's system properties. But wait a second! We can define these properties with run's params.

So the exploit technique is following:
1. Register the user and login
2. Upload a program which title has non-ASCII chars and overflow the class name, change base class to "h2/tools/Server".
3. Define "setMaze(String)" function in this program, otherwise running the code will fail before start
4. Call openBrowser("programs/")
5. Run this program with params "h2.browser": "ln,-sf,static/classes.tar.gz,%url"

This program will archive all class files by all participants and put it to static/classes.tar.gz, which is available via static file downloading.

