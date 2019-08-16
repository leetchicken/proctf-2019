namespace rubik

open System
open System.Security.Cryptography

open Helper
open Rubik

type RubikHelper() =
    [<Literal>] static let HmacSize = 20

    static member public Serialize(rubik: Rubik, key: Guid): string =
        let dataLength = sizeof<UInt16> + sizeof<UInt64> + sizeof<Guid> + rubik.Raw.Length
        let data = alloc<byte>(dataLength + HmacSize)

        BitConverter.TryWriteBytes(data, rubik.MovesCount) |> ignore
        BitConverter.TryWriteBytes(data.Slice(sizeof<UInt16>), rubik.Created.Ticks) |> ignore
        rubik.Id.TryWriteBytes(data.Slice(sizeof<UInt16> + sizeof<UInt64>)) |> ignore
        rubik.Raw.CopyTo(data.Slice(sizeof<UInt16> + sizeof<UInt64> + sizeof<Guid>))

        use hmac = new HMACSHA1(key.ToByteArray())
        hmac.TryComputeHash(data.Slice(0, dataLength).AsReadOnly(), data.Slice(dataLength)) |> ignore

        Convert.ToBase64String(data.AsReadOnly())

    static member public TryDeserialize(value: string, key: Guid) =
        match value with
        | value when value <> null && value.Length > 0 ->
            let length = GetFromBase64Length(value.Length)

            let mutable data = alloc<byte>(length)

            let result, bytes = Convert.TryFromBase64String(value, data)
            match result with
            | false -> Rubik.Empty
            | true ->
                data <- data.Slice(0, bytes)
                let hash = stackalloc<byte>(HmacSize)

                use hmac = new HMACSHA1(key.ToByteArray())
                let raw = data.Slice(0, sizeof<UInt16> + sizeof<UInt64> + sizeof<Guid> + Rubik.Size)
                match hmac.TryComputeHash(raw.AsReadOnly(), hash), hash.AsReadOnly().ConstantTimeEquals(data.Slice(data.Length - HmacSize).AsReadOnly(), fun c -> int(c)) with
                | (true, _), true -> new Rubik(Raw = raw.Slice(sizeof<UInt16> + sizeof<UInt64> + sizeof<Guid>), Created = new DateTime(BitConverter.ToInt64(data.Slice(sizeof<UInt16>).AsReadOnly())), Id = new Guid(data.Slice(sizeof<UInt16> + sizeof<UInt64>, sizeof<Guid>).AsReadOnly()))
                | _ -> Rubik.Empty

        | _ -> Rubik.Empty
