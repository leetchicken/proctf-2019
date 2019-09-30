package ae.hitb.proctf.drone_racing.programming

interface Interpreter<TState, TProgram, TInput> {
    fun initialState(input: TInput): TState
    fun join(s: TState, p: TProgram): TState
}

fun <TState, TProgram, TInput> Interpreter<TState, TProgram, TInput>.run(program: TProgram, input: TInput) =
        join(initialState(input), program)