package ae.hitb.proctf.drone_racing

import io.ktor.http.*
import io.ktor.server.testing.*
import io.ktor.util.KtorExperimentalAPI
import kotlin.test.*

@KtorExperimentalAPI
@ExperimentalStdlibApi
class ApplicationTest {
    @Test
    fun testRoot() {
        withTestApplication({ module(testing = true) }) {
            handleRequest(HttpMethod.Get, "/").apply {
                assertEquals(HttpStatusCode.OK, response.status())
                assertEquals("HELLO WORLD!", response.content)
            }
        }
    }
}
