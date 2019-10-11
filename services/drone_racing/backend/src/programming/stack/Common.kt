package ae.hitb.proctf.drone_racing.programming.stack

fun collectVariables(statements: Iterable<StackStatement>) = statements.flatMap {
    when (it) {
        is Ld -> listOf(it.v)
        is St -> listOf(it.v)
        is Pop -> listOf(poppedUnusedValueVariable)
        is Call -> emptyList()
        is TransEx -> listOf(currentExceptionVariable)
        else -> emptyList()
    }
}.distinct()