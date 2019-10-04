package ae.hitb.proctf.drone_racing

class Maze(val width: Int, val height: Int, private val mapString: String) {
    val map = mutableListOf<List<Boolean>>()

    init {
        check(mapString.length == width * height) { "Invalid size of map" }

        val row = mutableListOf<Boolean>()
        mapString.forEach { c ->
            row.add(c == '*')
            if (row.size == width) {
                map.add(row.toList())
                row.clear()
            }
        }
    }
}
