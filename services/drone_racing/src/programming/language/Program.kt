package ae.hitb.proctf.drone_racing.programming.language

import ae.hitb.proctf.drone_racing.programming.language.FunctionDeclaration

data class Program(val functionDeclarations: List<FunctionDeclaration>,
                   val mainFunction: FunctionDeclaration
)