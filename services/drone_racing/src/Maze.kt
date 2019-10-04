package ae.hitb.proctf.drone_racing

import ae.hitb.proctf.drone_racing.dao.getMapSizeFromString

class Maze(private val mapString: String) {
    val map = mutableListOf<List<Boolean>>()
    val size = getMapSizeFromString(mapString)

    init {
        val row = mutableListOf<Boolean>()
        mapString.forEach { c ->
            row.add(c == '*')
            if (row.size == size) {
                map.add(row.toList())
                row.clear()
            }
        }
    }
}
