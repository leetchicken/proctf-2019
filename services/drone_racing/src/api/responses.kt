package ae.hitb.proctf.drone_racing.api

import ae.hitb.proctf.drone_racing.dao.*

interface IApiResponse

data class OkResponse<out Body>(val response: Body, val status: String = "ok") where Body : IApiResponse
data class EmptyOkResponse(val status: String = "ok")
data class ErrorResponse(val message: String, val status: String = "error")
data class NotAuthenticatedResponse(val message: String = "You are not authenticated", val status: String = "error")

data class UsersListResponse(val users: List<User>) : IApiResponse
data class UserResponse(val user: User) : IApiResponse

data class LevelsResponse(val levels: List<Level>) : IApiResponse
data class LevelResponse(val level: Level) : IApiResponse

data class ProgramsResponse(val programs: List<Program>) : IApiResponse
data class ProgramResponse(val programId: Int) : IApiResponse

data class RunsResponse(val runs: List<Run>) : IApiResponse
data class RunResponse(val run: Run, val output: String, val error: Boolean, val errorMessage: String) : IApiResponse