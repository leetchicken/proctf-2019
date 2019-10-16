# rubik
Rubik's cube puzzle online web service. User can solve generated puzzles and see his own and others' results.

![rubik](img/rubik.png)

HTTP API contains 4 handlers:
* `/api/auth` — get current user auth info including private bio field;
* `/api/generate` — generate a new shuffled puzzle, response contains a puzzle for javascript UI and a serialized + HMAC'ed value which is posted back with the solution;
* `/api/solve` — POST to store the solution and possibly set auth cookie;
* `/api/scoreboard` — get latest solutions.

User can set no auth info to solve puzzle as a guest. If some info about user is provided server sets `AUTH` cookie and subsequent solves use this cookie to get user login and include it into solution results info. This cookie can be used to get info about user with `/api/auth`.

## Vulns
### 1. Stackalloc'ed buffer use-after-free
The stackalloc operator allocates a block of memory on the stack. The reason to use stackalloc is performance — fast allocation and deallocation, locality of data, less GC pressure, automatic cleanup — when the method ends — the stack gets unwinded together with its memory.
This cool new feature used in rubik service. The size of the stack is relatively small, thus we need some helper to get stack allocated small buffers and heap allocated the larger ones.

Here is the implementation of this approach:
```f#
let inline stackalloc<'a when 'a: unmanaged> size =
    let ptr = NativePtr.stackalloc<'a> size |> NativePtr.toVoidPtr
    Span<'a>(ptr, size)
    
let inline alloc<'a when 'a: unmanaged> size =
    if size < Small then stackalloc<'a>(size) else Span<'a>(Array.zeroCreate<'a>(size))
```

So, if the `size` is less than `Small` (100 bytes) the buffer will be stack allocated.
`Span<T>` is a simple value type that provide a type-safe and memory-safe way to work with any kind of contiguous memory region. It contains a pointer to this region.
The vuln is in a `TryDeserialize` method which returns a `Rubik` instance created from a `Span<byte>` possibly stack allocated. As a result, a pointer to the stack region is stored into an object, but this region will be freed after the method exits.
```f#
static member public TryDeserialize(value: string, key: Guid) =
    let mutable data = alloc<byte>(length)
    ...
    | ... -> new Rubik(Raw = data.Slice(sizeof<UInt16> + sizeof<UInt64> + sizeof<Guid>))
```

And then the saved memory pointer will be used in subsequent actions with the `Rubik` object.

### 2. Weakness of generated cube HMAC validation
A puzzle HMAC'ed with the key is serialized into an array of bytes without any delimiters and field lengths. During [deserialization](https://github.com/HackerDom/proctf-2019/blob/master/services/rubik/rubik/RubikHelper.fs#L42), the puzzle itself is read from the beginning of the array, and the hash is read from the end. Thus, if the end of the puzzle coincides with the beginning of the hash, then we can reduce the size of the value by the length of the match by throwing duplicate bytes. And the calculated hash remains valid. Fortunately, the length of the value is 100 and we need to reduce the length by only one byte so that our value gets onto the stack.

### Exploiting
After the `Rubik` instance deserialized there are 2 other memory regions [allocated](https://github.com/HackerDom/proctf-2019/blob/master/services/rubik/rubik/SolveHandler.fs#L51) on the stack: the first one is to store the `solution` — an array of puzzle turns, and a `key` that is used to encrypt cookies.

```f#
match RubikHelper.TryDeserialize(puzzle, SettingsManager.Current.Key) with
| rubik ->
    let moves = stackalloc<Rotation>(solution.Length)
    match SolveHandler.TryGetMoves(solution, moves) with
    | true ->
        let key = stackalloc<byte>(SettingsManager.Current.KeySize)
        SettingsManager.Current.Key.TryWriteBytes(key) |> ignore

            rubik.Turn(moves)
```

After all the memory on the stack is as follows:

![stack](img/stack.png)

So, by changing the length of the solution we can ensure that the memory regions of `rubik` and `key` overlap. It is necessary to take into account the alignment of memory regions. In addition, JIT can optimize compiled code that is used frequently, so the `key` offset relative to the `rubik` may change after several requests to the server.
You can try to determine `key` offset by checking HTTP status code of `/api/auth` request. Or you can modify your own service to see relative offsets in memory:
https://github.com/HackerDom/proctf-2019/blob/master/services/rubik/rubik/SolveHandler.fs#L60

The simplest way to select the right `solution` length is to debug the service and diff the `rubik` and the `key` offsets.
This is what the state of the puzzle looks like we'll use:

![puzzle](img/puzzle.png)

Rotation of the puzzle use the user specified array of turns. The server response doesn't contain the resulting puzzle or its parts, so we can't get the key bytes. But we can change arbitrary key bytes using the turns of the puzzle.
The idea is to leave a small (1..3) number of key bytes unknown, and change the rest by swapping with predefined bytes of the solution. After that, bruteforce the unknown bytes of the key. And then — pick up the next solution and bruteforce the next unknown bytes of the key.
An additional complication — the only decision bytes can act as known key bytes. And this bytes are sequentially used for turns. Therefore, it is necessary to determine such a safe solution prefix that will provide an invariant: at each turn the corresponding byte of the solution isn't corrupted by an unknown byte.
One of the possible safe `solution` prefixes that doesn't change the position of the facelets of the cube is `DDDDDDDDUUUUDDDDDdUUUUUUUUUUUUUUUU`.

![initial cube state](img/sln0.png)

Next, we need to create `solution` infix to align the key to the desired offset, it is convenient if then infix doesn't change the position of the facelets of the cube. For example: `DDDD UUUU ...` or `Ll Ff Uu ...`.
The last thing we need is to come up with a suffix that allows to bruteforce the key by 1..3 bytes. Below you can see the one of the possible variants of turns:

![step 1](img/sln1.png)
![step 2](img/sln2.png)
![step 3](img/sln3.png)
![step 4](img/sln4.png)
![step 5](img/sln5.png)
![step 6](img/sln6.png)

Using the reconstructed key we can forge cookies for other users who post solutions and get flags by calling the `/api/auth` method. See the exploit [here](https://github.com/HackerDom/proctf-2019/blob/master/sploits/rubik/src/Program.cs).
