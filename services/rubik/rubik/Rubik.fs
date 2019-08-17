module rubik.Rubik

open System
open System.Runtime.CompilerServices
open System.Runtime.InteropServices
open System.Security.Cryptography
open Microsoft.FSharp.Core.Operators

open Helper

type Color =
   | Red = 0x52uy
   | Green = 0x47uy
   | Blue = 0x42uy
   | White = 0x57uy
   | Yellow = 0x59uy
   | Black = 0x5Auy

type Rotation =
    | L = 0uy
    | F = 1uy
    | U = 2uy
    | R = 3uy
    | B = 4uy
    | D = 5uy

    | l = 6uy
    | f = 7uy
    | u = 8uy
    | r = 9uy
    | b = 10uy
    | d = 11uy

let LineSize = 3
let FacesCount = 6
let FaceSize = LineSize * LineSize
let Size = FacesCount * FaceSize

let Colors = [| Color.Red; Color.Green; Color.Blue; Color.White; Color.Yellow; Color.Black |]

[<Struct; StructLayout(LayoutKind.Explicit, Pack = 1, Size = 9)>]
type Face =
    [<FieldOffset(0)>] val mutable Sides: uint64
    [<FieldOffset(8)>] val mutable Center: byte

[<Struct; IsByRefLike; StructLayout(LayoutKind.Explicit, Pack = 1)>]
type Rubik =
    [<FieldOffset(0); DefaultValue>] val mutable MovesCount: uint16
    [<FieldOffset(8); DefaultValue>] val mutable Created: DateTime
    [<FieldOffset(16); DefaultValue>] val mutable Id: Guid

    [<FieldOffset(32); DefaultValue>] val mutable Raw: Span<byte>
    [<FieldOffset(32); DefaultValue>] val mutable Cube: Span<Color>
    [<FieldOffset(32); DefaultValue>] val mutable Faces: Span<Face>

    static member public Empty = new Rubik()

    member this.IsEmpty() =
        this.Created = Rubik.Empty.Created && this.MovesCount = Rubik.Empty.MovesCount && this.Raw.Length = 0

    static member public Init(): Rubik =
        let cube = new Span<Color>(Array.zeroCreate<Color>(FacesCount * FaceSize))
        for i = 0 to Colors.Length - 1 do
            cube.Slice(i * FaceSize, FaceSize).Fill(Colors.[i])
        new Rubik(Cube = cube, Id = Guid.NewGuid(), Created = DateTime.UtcNow)

    member this.IsCompleted(): bool =
        let mutable result = true
        for i = 0 to FacesCount - 1 do
            let idx = i * FaceSize
            let color = this.Raw.[idx]
            for j = idx + 1 to idx + FaceSize - 1 do
                result <- result && (this.Raw.[j] = color)
        result

    [<MethodImpl(MethodImplOptions.AggressiveInlining)>]
    member inline this.ToIndex(rotation: Rotation) =
        if rotation = Rotation.L then 0 else if rotation = Rotation.F then 1 else if rotation = Rotation.U then 2 else if rotation = Rotation.R then 3 else if rotation = Rotation.B then 4 else if rotation = Rotation.D then 5 else 0

    member inline this.RotateFaceClockwiseFast(rotation: Rotation) =
        Rol(&this.Faces.[this.ToIndex(rotation)].Sides, 16)

    member inline this.RotateFaceClockwise(idx: int) =
        let mutable tmp = this.Cube.[idx];
        this.Cube.[idx] <- this.Cube.[idx + 6];
        this.Cube.[idx + 6] <- this.Cube.[idx + 4];
        this.Cube.[idx + 4] <- this.Cube.[idx + 2];
        this.Cube.[idx + 2] <- tmp;

        tmp <- this.Cube.[idx + 1];
        this.Cube.[idx + 1] <- this.Cube.[idx + 7];
        this.Cube.[idx + 7] <- this.Cube.[idx + 5];
        this.Cube.[idx + 5] <- this.Cube.[idx + 3];
        this.Cube.[idx + 3] <- tmp;

    member inline this.TurnClockwise(rotation: Rotation) =
        this.RotateFaceClockwiseFast(rotation)
        match rotation with
        | Rotation.L ->
            let t1 = this.Cube.[9]
            let t2 = this.Cube.[16]
            let t3 = this.Cube.[15]
            this.Cube.[9] <- this.Cube.[18]
            this.Cube.[16] <- this.Cube.[25]
            this.Cube.[15] <- this.Cube.[24]
            this.Cube.[18] <- this.Cube.[40]
            this.Cube.[25] <- this.Cube.[39]
            this.Cube.[24] <- this.Cube.[38]
            this.Cube.[40] <- this.Cube.[45]
            this.Cube.[39] <- this.Cube.[52]
            this.Cube.[38] <- this.Cube.[51]
            this.Cube.[45] <- t1
            this.Cube.[52] <- t2
            this.Cube.[51] <- t3
        | Rotation.F ->
            let t1 = this.Cube.[4]
            let t2 = this.Cube.[3]
            let t3 = this.Cube.[2]
            this.Cube.[2] <- this.Cube.[45]
            this.Cube.[3] <- this.Cube.[46]
            this.Cube.[4] <- this.Cube.[47]
            this.Cube.[45] <- this.Cube.[33]
            this.Cube.[46] <- this.Cube.[34]
            this.Cube.[47] <- this.Cube.[27]
            this.Cube.[27] <- this.Cube.[24]
            this.Cube.[34] <- this.Cube.[23]
            this.Cube.[33] <- this.Cube.[22]
            this.Cube.[24] <- t1
            this.Cube.[23] <- t2
            this.Cube.[22] <- t3
        | Rotation.U ->
            let t1 = this.Cube.[0]
            let t2 = this.Cube.[1]
            let t3 = this.Cube.[2]
            this.Cube.[0] <- this.Cube.[9]
            this.Cube.[1] <- this.Cube.[10]
            this.Cube.[2] <- this.Cube.[11]
            this.Cube.[9] <- this.Cube.[27]
            this.Cube.[10] <- this.Cube.[28]
            this.Cube.[11] <- this.Cube.[29]
            this.Cube.[27] <- this.Cube.[36]
            this.Cube.[28] <- this.Cube.[37]
            this.Cube.[29] <- this.Cube.[38]
            this.Cube.[36] <- t1
            this.Cube.[37] <- t2
            this.Cube.[38] <- t3
        | Rotation.R ->
            let t1 = this.Cube.[11]
            let t2 = this.Cube.[12]
            let t3 = this.Cube.[13]
            this.Cube.[11] <- this.Cube.[47]
            this.Cube.[12] <- this.Cube.[48]
            this.Cube.[13] <- this.Cube.[49]
            this.Cube.[47] <- this.Cube.[42]
            this.Cube.[48] <- this.Cube.[43]
            this.Cube.[49] <- this.Cube.[36]
            this.Cube.[42] <- this.Cube.[20]
            this.Cube.[43] <- this.Cube.[21]
            this.Cube.[36] <- this.Cube.[22]
            this.Cube.[20] <- t1
            this.Cube.[21] <- t2
            this.Cube.[22] <- t3
        | Rotation.B ->
            let t1 = this.Cube.[0]
            let t2 = this.Cube.[7]
            let t3 = this.Cube.[6]
            this.Cube.[0] <- this.Cube.[20]
            this.Cube.[7] <- this.Cube.[19]
            this.Cube.[6] <- this.Cube.[18]
            this.Cube.[20] <- this.Cube.[31]
            this.Cube.[19] <- this.Cube.[30]
            this.Cube.[18] <- this.Cube.[29]
            this.Cube.[31] <- this.Cube.[51]
            this.Cube.[30] <- this.Cube.[50]
            this.Cube.[29] <- this.Cube.[49]
            this.Cube.[51] <- t1
            this.Cube.[50] <- t2
            this.Cube.[49] <- t3
        | Rotation.D ->
            let t1 = this.Cube.[6]
            let t2 = this.Cube.[5]
            let t3 = this.Cube.[4]
            this.Cube.[6] <- this.Cube.[42]
            this.Cube.[5] <- this.Cube.[41]
            this.Cube.[4] <- this.Cube.[40]
            this.Cube.[42] <- this.Cube.[33]
            this.Cube.[41] <- this.Cube.[32]
            this.Cube.[40] <- this.Cube.[31]
            this.Cube.[33] <- this.Cube.[15]
            this.Cube.[32] <- this.Cube.[14]
            this.Cube.[31] <- this.Cube.[13]
            this.Cube.[15] <- t1
            this.Cube.[14] <- t2
            this.Cube.[13] <- t3
        | _ -> ignore()
        this

    member inline this.TurnCounterClockwise(rotation: Rotation) =
        for i = 0 to 2 do
            this.TurnClockwise(rotation)
        this

    member inline this.Turn(move: Rotation) =
        this.MovesCount <- this.MovesCount + 1us
        if move < Rotation.l then this.TurnClockwise(move) else this.TurnCounterClockwise(move - Rotation.l)

    member inline this.Turn(moves: Span<Rotation>) =
        for i = 0 to moves.Length - 1 do
            this.Turn(moves.[i])

    member this.Shuffle(count: int): Rubik =
        for i = 0 to count - 1 do
            this.Turn(LanguagePrimitives.EnumOfValue(byte(RandomNumberGenerator.GetInt32(int(Rotation.l)))))
        this

    static member inline InitTestRubik(): Rubik =
        let raw = stackalloc<byte>(Size)
        for i = 0 to Size - 1 do
            raw.[i] <- byte(i)
        new Rubik(Raw = raw, Id = Guid.NewGuid(), Created = DateTime.UtcNow)

    static member Test(rotation: Rotation) =
        let inversed = rotation + Rotation.l

        let rubik1 = Rubik.InitTestRubik()
        let rubik2 = Rubik.InitTestRubik()

        rubik2.Turn(rotation)
        rubik2.Turn(inversed)
        assert(rubik1.Raw.AsReadOnly().SequenceEqual(rubik2.Raw.AsReadOnly()))

        rubik2.Turn(rotation)
        rubik2.Turn(rotation)
        rubik2.Turn(rotation)
        rubik2.Turn(rotation)
        assert(rubik1.Raw.AsReadOnly().SequenceEqual(rubik2.Raw.AsReadOnly()))

        rubik2.Turn(inversed)
        rubik2.Turn(inversed)
        rubik2.Turn(inversed)
        rubik2.Turn(inversed)
        assert(rubik1.Raw.AsReadOnly().SequenceEqual(rubik2.Raw.AsReadOnly()))

        rubik1.Turn(rotation)
        rubik1.Turn(rotation)
        rubik2.Turn(inversed)
        rubik2.Turn(inversed)
        assert(rubik1.Raw.AsReadOnly().SequenceEqual(rubik2.Raw.AsReadOnly()))

        let rubik = Rubik.Init()
        assert(rubik.IsCompleted())
        rubik.Turn(rotation)
        assert(not(rubik.IsCompleted()))
        rubik.Turn(inversed)
        assert(rubik.IsCompleted())

    static member public Test() =
        for rotation in [Rotation.L; Rotation.F; Rotation.U; Rotation.R; Rotation.B; Rotation.D] do
            Rubik.Test(rotation)
