package ae.hitb.proctf.drone_racing

import ae.hitb.proctf.drone_racing.api.*
import ae.hitb.proctf.drone_racing.dao.*
import com.google.gson.Gson
import com.google.gson.GsonBuilder
import freemarker.cache.ClassTemplateLoader
import io.ktor.application.*
import io.ktor.content.TextContent
import io.ktor.features.*
import io.ktor.freemarker.*
import io.ktor.gson.gson
import io.ktor.html.respondHtml
import io.ktor.http.*
import io.ktor.http.content.*
import io.ktor.request.*
import io.ktor.response.*
import io.ktor.routing.*
import io.ktor.sessions.*
import io.ktor.util.KtorExperimentalAPI
import io.ktor.util.date.GMTDate
import kotlinx.css.*
import kotlinx.html.*
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder
import java.nio.file.*
import java.text.DateFormat
import kotlin.collections.*
import kotlin.random.Random

fun main(args: Array<String>): Unit = io.ktor.server.netty.EngineMain.main(args)

val gson: Gson = GsonBuilder().setPrettyPrinting().setDateFormat(DateFormat.LONG).create()

@ExperimentalStdlibApi
@KtorExperimentalAPI
@Suppress("unused") // Referenced in application.conf
@kotlin.jvm.JvmOverloads
fun Application.module(testing: Boolean = false) {
    install(FreeMarker) {
        templateLoader = ClassTemplateLoader(this::class.java.classLoader, "templates")
    }

    install(Sessions) {
        val sessionKey = readOrGenerateSessionKey()
        cookie<Session>("session") {
            cookie.path = "/"
            cookie.httpOnly = true
            cookie.extensions["SameSite"] = "lax"
            transform(SessionTransportTransformerMessageAuthentication(sessionKey))
        }
    }

    install(Compression) {
        gzip {
            priority = 1.0
        }
        deflate {
            priority = 10.0
            minimumSize(1024) // condition
        }
    }

    install(AutoHeadResponse)

    install(CallLogging) {
        level = org.slf4j.event.Level.INFO
        filter { call -> call.request.path().startsWith("/") }
    }

    install(ConditionalHeaders)

    install(CORS) {
        method(HttpMethod.Options)
        method(HttpMethod.Put)
        method(HttpMethod.Delete)
        method(HttpMethod.Patch)
        header(HttpHeaders.Authorization)
        header("MyCustomHeader")
        allowCredentials = true
        anyHost() // @TODO: Don't do this in production if possible. Try to limit it.
    }

    install(CachingHeaders) {
        options { outgoingContent ->
            when (outgoingContent.contentType?.withoutParameters()) {
                ContentType.Text.CSS -> CachingOptions(CacheControl.MaxAge(maxAgeSeconds = 24 * 60 * 60), expires = null as? GMTDate?)
                else -> null
            }
        }
    }

    install(ContentNegotiation) {
        gson {
            setPrettyPrinting()
            setDateFormat(DateFormat.LONG)
        }
    }

    DatabaseFactory().init()

    routing {
        get("/") {
            call.respondText("HELLO WORLD!", contentType = ContentType.Text.Plain)
        }

        get("/html-dsl") {
            call.respondHtml {
                body {
                    h1 { +"HTML" }
                    ul {
                        for (n in 1..10) {
                            li { +"$n" }
                        }
                    }
                }
            }
        }

        get("/styles.css") {
            call.respondCss {
                body {
                    backgroundColor = Color.red
                }
                p {
                    fontSize = 2.em
                }
                rule("p.myclass") {
                    color = Color.blue
                }
            }
        }

        get("/html-freemarker") {
            call.respond(FreeMarkerContent("index.ftl", mapOf("data" to IndexData(listOf(1, 2, 3))), ""))
        }

        static("/static") {
            resources("static")
        }

        get("/json/gson") {
            call.respond(mapOf("hello" to "world"))
        }

        route("/api") {
            val userService = UserService(BCryptPasswordEncoder())

            route("/users") {
                post("") {
                    val request: CreateUserRequest
                    try {
                        request = call.receive()
                        checkNotNull(request.name) { "Please specify the name"}
                        checkNotNull(request.login) { "Please specify the login" }
                        checkNotNull(request.password) { "Please specify the password" }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@post
                    }

                    try {
                        val user = userService.createUser(request.name, request.login, request.password)
                        call.respond(OkResponse(UserResponse(user)))
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Can't create user: ${e.message}"))
                        return@post
                    }
                }

                post("/login") {
                    val request: LoginRequest
                    try {
                        request = call.receive()
                        checkNotNull(request.login) { "Please specify the login" }
                        checkNotNull(request.password) { "Please specify the password" }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@post
                    }

                    val user = userService.authenticate(request.login, request.password)
                    if (user == null) {
                        call.respond(ErrorResponse("Login or password is incorrect"))
                        return@post
                    }

                    call.sessions.set(Session(user.id.value))
                    call.respond(OkResponse(UserResponse(user)))
                }

                post("/logout") {
                    call.sessions.set(Session(-1))
                    call.respond(EmptyOkResponse())
                }

                get("/me") {
                    val session = call.sessions.get<Session>()
                    if (session == null || session.userId < 0) {
                        call.respond(HttpStatusCode.Unauthorized, NotAuthenticatedResponse())
                        return@get
                    }

                    val user = userService.findUserById(session.userId)
                    if (user == null) {
                        call.respond(HttpStatusCode.Unauthorized, NotAuthenticatedResponse())
                        return@get
                    }

                    call.respond(OkResponse(UserResponse(user)))
                }
            }

            var authenticatedUser: User? = null

            intercept(ApplicationCallPipeline.Call) {
                if (call.request.uri.startsWith("/api/users"))
                    return@intercept

                val session = call.sessions.get<Session>()
                if (session == null) {
                    call.respond(HttpStatusCode.Unauthorized, NotAuthenticatedResponse())
                    finish()
                    return@intercept
                }
                val userId = session.userId
                authenticatedUser = userService.findUserById(userId)
                if (authenticatedUser == null) {
                    call.respond(HttpStatusCode.Unauthorized, NotAuthenticatedResponse())
                    finish()
                    return@intercept
                }
            }

            route("/levels") {
                val levelService = LevelService()

                get {
                    val levels = levelService.getLevels()
                    call.respond(OkResponse(LevelsResponse(levels)))
                    return@get
                }

                get("{id}") {
                    val levelId = call.parameters["id"]?.toInt()
                    if (levelId == null) {
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid level id"))
                        return@get
                    }
                    val level = levelService.findLevelById(levelId)
                    if (level == null) {
                        call.respond(HttpStatusCode.NotFound, ErrorResponse("Invalid level id"))
                        return@get
                    }
                    call.respond(OkResponse(LevelResponse(level)))
                    return@get
                }

                post {
                    val request: CreateLevelRequest
                    try {
                        request = call.receive()
                        checkNotNull(request.title) { "Please specify the title" }
                        checkNotNull(request.map) { "Please specify the map" }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@post
                    }

                    val level: Level
                    try {
                        with (request) {
                            val size = getMapSizeFromString(request.map)
                            check(map.length == size * size) { "Map length should be a square "}
                            check(size in 1..LEVEL_MAX_SIZE) { "Map's size should be more than 0 and less than $LEVEL_MAX_SIZE"}
                            check(map.all { it in ".*" }) { "Map should contain only '.' and '*' chars "}
                            level = levelService.createLevel(authenticatedUser!!, title, map)
                        }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Can't create level: ${e.message}"))
                        return@post
                    }

                    call.respond(OkResponse(LevelResponse(level)))
                }
            }

            route("/programs") {
                val levelService = LevelService()
                val programService = ProgramService()

                post {
                    val request: CreateProgramRequest
                    val level: Level
                    try {
                        request = call.receive()
                        checkNotNull(request.levelId) { "please specify the level" }
                        checkNotNull(request.title) { "please specify the title" }
                        checkNotNull(request.sourceCode) { "please specify the source code" }
                        level = levelService.findLevelById(request.levelId) ?: throw IllegalStateException("unknown level id")
                        check(request.sourceCode.length in 1..PROGRAM_MAX_SIZE) { "source code size should by more than 0 bytes and not more than $PROGRAM_MAX_SIZE bytes" }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@post
                    }

                    try {
                        val program = programService.createProgram(authenticatedUser!!, level, request.title, request.sourceCode)
                        call.respond(OkResponse(ProgramIdResponse(program.id.value)))
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                    }
                }

                get {
                    val level: Level
                    try {
                        val levelId = call.parameters["level_id"]?.toInt() ?: throw IllegalStateException("invalid level id")
                        level = levelService.findLevelById(levelId) ?: throw IllegalStateException("unknown level id")
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@get
                    }

                    var result: String? = null
                    dbQuery {
                        val programs = programService.findUserPrograms(authenticatedUser!!, level)
                        result = gson.toJson(OkResponse(ProgramsResponse(programs)))
                    }
                    call.respond(TextContent(result!!, ContentType.Application.Json))
                }

                get("{id}") {
                    val programId = call.parameters["id"]?.toInt()
                    val program: Program?;
                    try {
                        checkNotNull(programId) { "invalid program id" }
                        program = programService.findProgramById(programId)
                        checkNotNull(program) { "unknown program id" }
                        dbQuery {
                            check(program.author.id == authenticatedUser?.id) { "it's not your program, sorry" }
                        }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@get
                    }

                    var result: String? = null
                    dbQuery {
                        result = gson.toJson(OkResponse(ProgramResponse(program)))
                    }
                    call.respond(TextContent(result!!, ContentType.Application.Json))
                }
            }

            route("/runs") {
                val levelService = LevelService()
                val programService = ProgramService()
                val runService = RunService()

                get {
                    val level: Level
                    try {
                        val levelId = call.parameters["level_id"]?.toInt() ?: throw IllegalStateException("invalid level id")
                        level = levelService.findLevelById(levelId) ?: throw IllegalStateException("unknown level id")
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@get
                    }

                    var result: String? = null
                    dbQuery {
                        val runs = runService.findSuccessRunsOnLevel(level)
                        result = gson.toJson(OkResponse(RunsResponse(runs)))
                    }
                    call.respond(TextContent(result!!, ContentType.Application.Json))
                }

                post {
                    val request: CreateRunRequest
                    val program: Program?
                    var level: Level? = null
                    try {
                        request = call.receive()
                        checkNotNull(request.programId) { "please specify the program id" }
                        checkNotNull(request.params) { "please specify the params list" }
                        program = programService.findProgramById(request.programId)
                        checkNotNull(program) { "unknown program id" }
                        dbQuery {
                            check(program.author.id == authenticatedUser?.id) { "it's not your program, sorry" }
                        }
                        dbQuery {
                            level = program.level
                        }
                        checkNotNull(level) { "Can't find level for your program" }
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.BadRequest, ErrorResponse("Invalid request: ${e.message}"))
                        return@post
                    }

                    val startTime = getUnixTimestamp()

                    val runResult: RunResult
                    try {
                        runResult = CodeRunner().runCode(
                            Paths.get(PROGRAMS_PATH, program.file).toString(),
                            request.params,
                            level!!.map
                        )
                    } catch (e: Throwable) {
                        e.printStackTrace()
                        call.respond(HttpStatusCode.InternalServerError, ErrorResponse("Can't run your program, sorry"))
                        return@post
                    }

                    val finishTime = getUnixTimestamp()
                    var result: String? = null
                    dbQuery {
                        val run = runService.createRun(program, startTime, finishTime, runResult.success, runResult.score)
                        result = gson.toJson(OkResponse(RunResponse(run, runResult.output, runResult.error, runResult.errorMessage)))
                    }

                    call.respond(TextContent(result!!, ContentType.Application.Json))
                }
            }
        }
    }
}

data class IndexData(val items: List<Int>)

data class Session(val userId: Int = -1)

fun FlowOrMetaDataContent.styleCss(builder: CSSBuilder.() -> Unit) {
    style(type = ContentType.Text.CSS.toString()) {
        +CSSBuilder().apply(builder).toString()
    }
}

fun CommonAttributeGroupFacade.style(builder: CSSBuilder.() -> Unit) {
    this.style = CSSBuilder().apply(builder).toString().trim()
}

suspend inline fun ApplicationCall.respondCss(builder: CSSBuilder.() -> Unit) {
    this.respondText(CSSBuilder().apply(builder).toString(), ContentType.Text.CSS)
}

fun readOrGenerateSessionKey() : ByteArray {
    val path = Paths.get("session.key")
    if (Files.exists(path) and Files.isReadable(path))
        return Files.readAllBytes(path)

    val sessionKey = Random.Default.nextBytes(10)
    Files.write(path, sessionKey)
    return sessionKey
}

fun getUnixTimestamp() = System.currentTimeMillis()