module rubik.Helper

open System
open System.Runtime.CompilerServices
open System.Threading
open Microsoft.FSharp.NativeInterop

let inline (!>) (x: ^a): ^b = ((^a or ^b): (static member op_Implicit : ^a -> ^b) x)

[<Extension>]
type String with
    [<Extension>]
    member inline this.TrimOrNull() = if this <> null then this.Trim() else null

#if HACK
open System.Runtime.InteropServices

[<Struct; IsByRefLike; StructLayout(LayoutKind.Explicit)>]
type CastToPointerStruct =
    [<FieldOffset(0)>]
    val mutable Span: Span<byte>

    [<DefaultValue; FieldOffset(0)>]
    val Pointer: uint64

    new(span: Span<byte>) = { Span = span }

    member inline this.GetPointer() =
        this.Pointer
#endif

type ThreadStaticRnd(random: Random) =
    static let instance = new ThreadLocal<_>(fun () -> ThreadStaticRnd(new Random(Guid.NewGuid().GetHashCode())))
    static member Instance = instance.Value
    member __.Random = random

[<Literal>]
let Small = 100

type Span<'a> with
    member x.AsReadOnly() = Span<_>.op_Implicit(x)

[<MethodImpl(MethodImplOptions.NoOptimization)>]
type ReadOnlySpan<'a> with
    member x.ConstantTimeEquals(other: ReadOnlySpan<'a>, cast: 'a -> int) =
        match x.Length = other.Length with
        | true ->
            let mutable result = 0
            for i = 0 to x.Length - 1 do
                result <- result ||| cast(x.[i]) ^^^ cast(other.[i])
            result = 0
        | _ -> false

let inline stackalloc<'a when 'a: unmanaged> size =
    let ptr = NativePtr.stackalloc<'a> size |> NativePtr.toVoidPtr
    Span<'a>(ptr, size)

let inline alloc<'a when 'a: unmanaged> size =
    if size < Small then stackalloc<'a>(size) else Span<'a>(Array.zeroCreate<'a>(size))


let inline GetToBase64Length(len: int): int =
    ((len <<< 2) / 3 + 3) &&& ~~~3

let inline GetFromBase64Length(len: int): int =
    len / 4 * 3


let inline Rol(value: UInt64 byref, bits: int) =
    value <- (value <<< bits) ||| (value >>> (64 - bits))
