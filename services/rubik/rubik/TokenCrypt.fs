namespace rubik

open System
open System.Security.Cryptography

open Helper

type TokenCrypt() =
    [<Literal>] static let TagSize = 12
    [<Literal>] static let NonceSize = 12

    static member CalcEncryptedLength(length: int) =
        GetToBase64Length(length + TagSize + NonceSize)

    static member CalcDecryptedLength(length: int) =
        GetFromBase64Length(length) - TagSize - NonceSize

    static member EncryptAndMac(key: ReadOnlySpan<byte>, plain: ReadOnlySpan<byte>, token: Span<char> byref) =
        let data = alloc<byte>(TagSize + NonceSize + plain.Length)

        let tag = data.Slice(0, TagSize)
        let nonce = data.Slice(TagSize, NonceSize)
        let cipher = data.Slice(TagSize + NonceSize)

        RandomNumberGenerator.Fill(nonce)

        use aes = new AesGcm(key)
        aes.Encrypt(nonce.AsReadOnly(), plain, cipher, tag)

        let mutable chars = 0
        match Convert.TryToBase64Chars(data.AsReadOnly(), token, &chars) with
        | true -> token <- token.Slice(0, chars);
        | false -> raise (Exception("Token is too short"))

    static member CheckMacAndDecrypt(key: ReadOnlySpan<byte>, token: ReadOnlySpan<char>, plain: Span<byte> byref) =
        let data = alloc<byte>(plain.Length + NonceSize + TagSize)

        let mutable bytes = 0
        match Convert.TryFromBase64Chars(token, data, &bytes) with
        | false -> raise (Exception("Invalid token"))
        | true ->
            let size = bytes - TagSize - NonceSize
            plain <- plain.Slice(0, size);

            let tag = data.Slice(0, TagSize)
            let nonce = data.Slice(TagSize, NonceSize)
            let cipher = data.Slice(TagSize + NonceSize, size)

            use aes = new AesGcm(key)
            aes.Decrypt(nonce.AsReadOnly(), cipher.AsReadOnly(), tag.AsReadOnly(), plain)
