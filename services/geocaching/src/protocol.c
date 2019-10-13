#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#include "base64.h"
#include "db.h"
#include "debug.h"
#include "geocacher.pb-c.h"
#include "mpilib.h"

#include "rsa_keys.h"

const size_t BUF_SIZE = 32767;

typedef struct __attribute__((__packed__))
{
    unsigned int msg_id;    // protobuf message type identifier
    unsigned int seq_id;    // message sequence number, increments by 1 for client messages,
                            // always 0 for server messages.
    unsigned char flags;    // `1` means that the packet is encrypted,
                            // `2` means that the packet is signed
    unsigned int data_size; // length of protobuf payload in bytes

    // char signature[32] — optional, used if the message is signed
    // char iv[16] — optional, used if the message is encrypted

    unsigned char data[];   // protobuf data
} Packet;


typedef struct
{
    unsigned char aes_key[16];
    unsigned int sequence_id;
    unsigned char hmac1[32];
    unsigned char session_key[32];
    unsigned char admin_challenge[64];
    unsigned int admin;
    unsigned int admin_challenge_size;
} ClientState;

ClientState state;

void openssl_rsa_decrypt(unsigned char *cipherTextBytes, size_t cipherTextBytes_len,
                         unsigned char *plainTextBytes, size_t *plainTextSize)
{
    RSA *privkey = RSA_new();
    RSA_set0_key(privkey, BN_bin2bn(N, 128, NULL), BN_bin2bn(E, 3, NULL), BN_bin2bn(D, 128, NULL));

    int bytesRead = RSA_private_decrypt(RSA_size(privkey),
                                        cipherTextBytes,
                                        plainTextBytes,
                                        privkey,
                                        RSA_PKCS1_OAEP_PADDING);
    if (bytesRead == -1)
    {
        exit(12031203);
    }

    RSA_free(privkey);

    *plainTextSize = bytesRead;
}

void aes_crypt(unsigned char *out, size_t *out_size, unsigned char *in, size_t in_size,
               unsigned char *key, unsigned char *orig_iv, unsigned int key_size, int mode)
{
    AES_KEY aes_key;
    unsigned char iv[16];
    memcpy(iv, orig_iv, 16);
    if (mode == 1)
    {
        *out_size = ((in_size / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        AES_set_encrypt_key(key, 8 * key_size, &aes_key);
        unsigned char *padded = malloc(*out_size);
        memcpy(padded, in, in_size);
        for (int i = in_size; i < *out_size; i++)
        {
            padded[i] = (*out_size - in_size);
        }
        AES_cbc_encrypt(padded, out, *out_size, &aes_key, iv, mode);
        free(padded);
    }
    else
    {
        AES_set_decrypt_key(key, 8 * key_size, &aes_key);
        AES_cbc_encrypt(in, out, in_size, &aes_key, iv, mode);
        *out_size -= out[*out_size - 1]; // TODO: handle padding correctly
    }
}

void compute_hmac(unsigned char *key, unsigned int key_len, unsigned char*data,
                  unsigned int data_len, unsigned char *result, unsigned int* result_len)
{
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (!ctx)
    {
        return;
    }
    HMAC_Init_ex(ctx, key, key_len, EVP_sha256(), 0);
    HMAC_Update(ctx, data, data_len);
    *result_len = 0;
    HMAC_Final(ctx, result, result_len);
    if (*result_len < 32)
    {
        result[*result_len] = 0;
    }
    HMAC_CTX_free(ctx);
}

int need_to_encrypt(Packet *packet)
{
    return (packet->msg_id < 15 || packet->msg_id > 17);
}

Packet *packet_builder(unsigned int message_id, void *msg, size_t *out_size)
{
    Packet *packet = 0;
    unsigned int len = 0;
    unsigned char *buf = NULL;
    if (message_id == 3)
    {
        len = store_secret_response__get_packed_size((StoreSecretResponse *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        store_secret_response__pack((StoreSecretResponse *)msg, packet->data);
    }
    else if (message_id == 5)
    {
        len = get_secret_response__get_packed_size((GetSecretResponse *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        get_secret_response__pack((GetSecretResponse *)msg, packet->data);
    }
    else if (message_id == 7)
    {
        len = discard_secret_response__get_packed_size((DiscardSecretResponse *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        discard_secret_response__pack((DiscardSecretResponse *)msg, packet->data);
    }
    else if (message_id == 11)
    {
        len = list_all_busy_cells_response__get_packed_size((ListAllBusyCellsResponse *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        list_all_busy_cells_response__pack((ListAllBusyCellsResponse *)msg, packet->data);
    }
    else if (message_id == 12)
    {
        len = admin_challenge__get_packed_size((AdminChallenge *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        admin_challenge__pack((AdminChallenge *)msg, packet->data);
    }
    else if (message_id == 14)
    {
        len = unknown_message__get_packed_size((UnknownMessage *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        unknown_message__pack((UnknownMessage *)msg, packet->data);
    }
    else if (message_id == 16)
    {
        len = auth_response__get_packed_size((AuthResponse *)msg);
        *out_size = sizeof(Packet) + len;
        packet = (Packet *)malloc(*out_size);
        auth_response__pack((AuthResponse *)msg, packet->data);
    }
    else
    {
        LOG("Fatal: trying to send unexpected packet\n");
        exit(1);
    }
    packet->msg_id = message_id;
    packet->seq_id = 0;
    packet->flags = 0;
    packet->data_size = len;

    unsigned char *data_ptr = packet->data;

    unsigned char *encrypted_data = data_ptr;
    size_t encrypted_data_size = packet->data_size;

    if (need_to_encrypt(packet))
    {
        packet->flags |= 1;
        unsigned char iv[16];
        getrandom(iv, 16, 0);
        encrypted_data_size = ((packet->data_size / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        encrypted_data = malloc(encrypted_data_size);
        aes_crypt(encrypted_data, &encrypted_data_size,
                  data_ptr, packet->data_size, state.session_key, iv, 32, 1);
        unsigned char *packet2 = malloc(sizeof(Packet) + 16 + encrypted_data_size);
        memcpy(packet2, packet, sizeof(Packet));
        memcpy(&packet2[sizeof(Packet)], iv, 16);
        memcpy(&packet2[sizeof(Packet) + 16], encrypted_data, encrypted_data_size);
        packet = (Packet *)packet2;
        *out_size = sizeof(Packet) + 16 + encrypted_data_size;
    }

    return packet;
}

void send_auth_response(unsigned char *message, unsigned int response_size)
{
    AuthResponse msg = AUTH_RESPONSE__INIT;
    msg.auth_key.data = message;
    msg.auth_key.len = response_size;
    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_store_secret_response(Status status, unsigned char *key)
{
    StoreSecretResponse msg = STORE_SECRET_RESPONSE__INIT;
    msg.status = status;
    msg.key.len = STORAGE_PASSWORD_LENGTH;
    msg.key.data = key;
    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_get_secret_response(Status status, unsigned char *resultt)
{
    GetSecretResponse msg = GET_SECRET_RESPONSE__INIT;
    msg.status = status;
    if (status == STATUS__OK)
    {
        msg.secret.len = strlen(resultt);
        msg.secret.data = resultt;
    }
    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_discard_secret_response(Status status)
{
    DiscardSecretResponse msg = DISCARD_SECRET_RESPONSE__INIT;
    msg.status = status;
    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_unknown_message(unsigned char *message)
{
    UnknownMessage msg = UNKNOWN_MESSAGE__INIT;
    msg.data.len = strlen(message);
    msg.data.data = message;
    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_admin_challenge(unsigned int challenge_size)
{
    AdminChallenge msg = ADMIN_CHALLENGE__INIT;

    msg.data.len = challenge_size;
    msg.data.data = state.admin_challenge;
    state.admin_challenge_size = challenge_size;
    getrandom(state.admin_challenge, challenge_size, 0);

    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

void send_list_all_busy_cells_response()
{
    ListAllBusyCellsResponse msg = LIST_ALL_BUSY_CELLS_RESPONSE__INIT;

    unsigned int n = get_number_of_flags_to_list();

    msg.n_cells = n;
    msg.cells = malloc(sizeof(Cell *) * n);
    for (int i = 0; i < n; i++) {
        msg.cells[i] = malloc(sizeof(Cell));
        cell__init(msg.cells[i]);
        msg.cells[i]->coordinates = malloc(sizeof(Coordinate));
        coordinate__init(msg.cells[i]->coordinates);
    }

    msg.status = list_flags(&msg);
    if (msg.status != STATUS__OK) {
        msg.n_cells = 0;
    }

    size_t packet_size = 0;
    Packet *packet = packet_builder(msg.message_type, &msg, &packet_size);
    size_t out_size = 0;
    unsigned char *result = base64_encode((unsigned char *) packet, packet_size, &out_size);
    puts(result);
    free(result);
    free(packet);
}

int check_admin_response(AdminResponse *msg)
{
    unsigned char result[128];
    unsigned char test_data[128];
    memset(test_data, 0, 128);
    memcpy(test_data, msg->data.data, msg->data.len);

    set_precision(64);
    mp_modexp((unitptr)&result, (unitptr)&test_data, (unitptr)&ADMIN_E, (unitptr)&ADMIN_N);

    for (int i = 0; i < 3; i++)
    {
        if (result[126 - i] != state.session_key[i])
        {
            return 0;
        }
    }

    if (state.admin_challenge_size == 0) {
        return 0;
    }

    for (int i = 0; i < state.admin_challenge_size; i++)
    {
        if (result[123 - i] != state.admin_challenge[i])
        {
            return 0;
        }
    }
    LOG("Admin challenge passed\n");
    return 1;
}

void handle_packet(Packet *packet, void *mmsg)
{
    LOG("Handling the packet\n");
    if (packet->msg_id == 2)
    {
        unsigned char password[STORAGE_PASSWORD_LENGTH];
        getrandom(password, STORAGE_PASSWORD_LENGTH, 0);
        Status status = put_flag((StoreSecretRequest *)mmsg, password);
        send_store_secret_response(status, password);
    }
    else if (packet->msg_id == 4)
    {
        unsigned char *resultt;
        Status status = get_flag((GetSecretRequest *)mmsg, &resultt);
        send_get_secret_response(status, resultt);
    }
    else if (packet->msg_id == 6)
    {

        Status status = delete_flag((DiscardSecretRequest *)mmsg);
        send_discard_secret_response(status);
    }
    else if (packet->msg_id == 10)
    {
        if (!state.admin)
        {
            send_admin_challenge(3);
        }
        else
        {
            send_list_all_busy_cells_response();
        }
    }
    else if (packet->msg_id == 13)
    {
        if (check_admin_response((AdminResponse *)mmsg))
        {
            state.admin = 1;
            send_list_all_busy_cells_response();
            state.admin = 0;
        }
    }
    else if (packet->msg_id == 15)
    {
        AuthRequest *msg = (AuthRequest *)mmsg;
        size_t out_size = 0;
        unsigned char decrypted[128];
        size_t decrypted_size;
        openssl_rsa_decrypt(msg->data.data, msg->data.len, &decrypted[0], &decrypted_size);

        unsigned char response[32];
        size_t response_size = 0;
        unsigned char iv[16];
        getrandom(iv, 16, 0);

        aes_crypt(response, &response_size, &decrypted[16], 16, decrypted, iv, 16, 1);

        unsigned char *real_response = malloc(response_size + 16);
        memcpy(real_response, response, response_size);
        memcpy(real_response + response_size, iv, 16);

        memcpy(state.aes_key, decrypted, 16);

        send_auth_response(real_response, response_size + 16);
    }
    else if (packet->msg_id == 17)
    {
        AuthResult *msg = (AuthResult *)mmsg;
        if (msg->status != STATUS__OK)
        {
            LOG("Secure session aborted: %d\n", msg->status);
            exit(1);
        }
        unsigned int key1_size = 0;
        unsigned int key2_size = 0;
        compute_hmac(state.aes_key, 16, "11111111111111111111111111111111", 32,
                     state.hmac1, &key1_size);
        compute_hmac(state.aes_key, 16, "22222222222222222222222222222222", 32,
                     state.session_key, &key2_size);
        state.sequence_id = 1;
    }
}

void cleanup_packet(Packet *packet, void *msg)
{
    if (packet->msg_id == 2)
    {
        store_secret_request__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 4)
    {
        get_secret_request__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 6)
    {
        discard_secret_request__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 10)
    {
        list_all_busy_cells_request__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 13)
    {
        admin_response__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 15)
    {
        auth_request__free_unpacked(msg, NULL);
    }
    else if (packet->msg_id == 17)
    {
        auth_result__free_unpacked(msg, NULL);
    }

    free(packet);
}

void *packet_parser(Packet *packet)
{
    LOG("Incoming packet: %d %d %d\n", packet->msg_id, packet->seq_id, packet->flags);
    unsigned char *data_ptr = packet->data;
    if (packet->flags & 2)
    {
        if (packet->seq_id != state.sequence_id++)
        {
            printf("Aborting session: out of order packet, sequence number %d, expected %d.\n",
                   packet->seq_id, state.sequence_id - 1);
            exit(1);
        }
    }

    unsigned char hmac_input[512];
    *(unsigned int *)(&hmac_input[0]) = packet->msg_id;
    *(unsigned int *)(&hmac_input[4]) = packet->seq_id;
    *(unsigned int *)(&hmac_input[8]) = packet->msg_id;
    hmac_input[12] = packet->flags;

    if (packet->flags & 1)
    {
        data_ptr += 16;
    }
    if (packet->flags & 2)
    {
        data_ptr += 32;
    }

    unsigned char *decrypted_data = data_ptr;
    size_t decrypted_data_size = packet->data_size;
    if (!(packet->flags & 1) && need_to_encrypt(packet))
    {
        printf("Aborting session: unencrypted packet received.\n");
        exit(1);
    }

    if (packet->flags & 1)
    {
        decrypted_data = malloc(decrypted_data_size);
        aes_crypt(decrypted_data, &decrypted_data_size,
                  data_ptr, packet->data_size, state.session_key, data_ptr - 16, 32, 0);
    }

    size_t hmaced_data_len = decrypted_data_size;
    if (hmaced_data_len > 260)
    {
        hmaced_data_len = 260;
    }
    memcpy(&hmac_input[13], decrypted_data, hmaced_data_len);
    hmaced_data_len += 13;

    if (packet->flags & 1)
    {
        memcpy(&hmac_input[hmaced_data_len], data_ptr - 16, 16);
        hmaced_data_len += 16;
    }

    if (packet->flags & 2)
    {
        unsigned char hmac_result[32];
        unsigned int hmac_result_len = 0;
        compute_hmac(state.hmac1, 32, hmac_input, hmaced_data_len,
                     &hmac_result[0], &hmac_result_len);

        if (memcmp(hmac_result, packet->data, 32))
        {
            printf("Aborting session: Wrong packet signature\n");
            exit(1);
        }
    }

    void *msg = 0;
    if (packet->msg_id == 2)
    {
        LOG("Unpacking StoreSecretRequest\n");
        msg = (void *)store_secret_request__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 4)
    {
        LOG("Unpacking GetSecretRequest\n");
        msg = (void *)get_secret_request__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 6)
    {
        LOG("Unpacking DiscardSecretRequest\n");
        msg = (void *)discard_secret_request__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 10)
    {
        LOG("Unpacking ListAllBusyCellsRequest\n");
        msg = (void *)list_all_busy_cells_request__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 13)
    {
        LOG("Unpacking AdminResponse\n");
        msg = (void *)admin_response__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 15)
    {
        LOG("Unpacking AuthRequest\n");
        msg = (void *)auth_request__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else if (packet->msg_id == 17)
    {
        LOG("Unpacking AuthResult\n");
        msg = (void *)auth_result__unpack(NULL, decrypted_data_size, decrypted_data);
    }
    else
    {
        send_unknown_message("I don't understand you");
        return NULL;
    }

    if (msg == NULL)
    {
        printf("Aborting session: Could not unpack incoming message\n");
        exit(1);
    }
    return msg;
}

void parse_line(char *line)
{
    size_t out_size = 0;
    Packet *packet = (Packet *)base64_decode(line, strlen(line), &out_size);
    void *msg = packet_parser(packet);
    if (msg)
    {
        handle_packet(packet, msg);
        cleanup_packet(packet, msg);
    }
}

void prompt()
{
    printf("> ");
}

void print_greeting()
{
    puts(".    .    *  .   .  .   .  *     .  .        . .   .     .  *   .     .  .   .");
    puts("   *  .    .    *  .     .         .    * .     .  *  .    .   .   *   . .    .");
    puts(". *      .   .    .  .     .  *      .      .        .     .-o--.   .    *  .");
    puts(" .  .        .     .     .      .    .     *      *   .   :O o O :      .     .");
    puts("____   *   .    .      .   .           .  .   .      .    : O. Oo;    .   .");
    puts(" `. ````.---...___      .      *    .      .       .   * . `-.O-'  .     * . .");
    puts("   \\_    ;   \\`.-'```--..__.       .    .      * .     .       .     .        .");
    puts("   ,'_,-' _,-'             ``--._    .   *   .   .  .       .   *   .     .  .");
    puts("   -'  ,-'                       `-._ *     .       .   *  .           .    .");
    puts("    ,-'            _,-._            ,`-. .    .   .     .      .     *    .   .");
    puts("    '--.     _ _.._`-.  `-._        |   `_   .      *  .    .   .     .  .    .");
    puts("        ;  ,' ' _  `._`._   `.      `,-''  `-.     .    .     .    .      .  .");
    puts("     ,-'   \\    `;.   `. ;`   `._  _/\\___     `.       .    *     .    . *");
    puts("     \\      \\ ,  `-'    )        `':_  ; \\      `. . *     .        .    .    *");
    puts("      \\    _; `       ,;               __;        `. .           .   .     . .");
    puts("       '-.;        __,  `   _,-'-.--'''  \\-:        `.   *   .    .  .   *   .");
    puts("          )`-..---'   `---''              \\ `.        . .   .  .       . .  .");
    puts("        .'                                 `. `.       `  .    *   .      .  .");
    puts("       /                                     `. `.      ` *          .       .");
    puts("      /                                        `. `.     '      .   .     *");
    puts("     /                                           `. `.   _'.  .       .  .    .");
    puts("    |                                              `._\\-'  '     .        .  .");
    puts("    |                                                 `.__, \\  *     .   . *. .");
    puts("    |                                                      \\ \\.    .         .");
    puts("    |                                                       \\ \\ .     * jrei  *");
    puts("");
    puts("                         ::: Welcome to Deposit Box :::");
    puts("");
    puts("The unique climate and proximity to major galactic hubs makes Deposit Box the");
    puts("leading planet for storing your goods.");
    puts("");
    puts("Are you traveling in hyperspace and need to store some junk to save on mass?");
    puts("Are you a bounty hunter and prisoners take up precious space in your boot?");
    puts("Your Death Star project was put on hold due to financial problems?");
    puts("");
    puts("We've got you covered!");
    puts("Money, documents, animals, sentients, spacecrafts - we store anything!");
    puts("First two eons of storage are free!");
    puts("");
}

void init_client_state()
{
    memset(&state, 0, sizeof(state));
    state.sequence_id = 1;
}

void print_help()
{
    puts("The planet is split into individual cells, each cell is addressable");
    puts("by longitude and latitude.");
    puts("");
    puts("To access your cell you will need an individual access code.");
    puts("");
    puts("The official language of the planet is Baserian.");
    puts("");
    puts("Have a nice day!");
}

void handle_request()
{
    char buf[BUF_SIZE];

    prompt();

    if (!fgets(buf, BUF_SIZE, stdin))  // read until \n or until BUF_SIZE
    {
        dump_fast_storage();
        exit(0);
    }
    strtok(buf, "\r\n");             // Skip everything after "\r" or "\n"

    if (!strcmp(buf, "help"))
    {
        print_help();
    }
    else
    {
        parse_line(buf);
    }

    puts("");
}
