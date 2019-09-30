package ae.hitb.proctf.drone_racing.programming

interface Compiler<TSource, TTarget> {
    fun compile(programName: String, source: TSource): TTarget
}