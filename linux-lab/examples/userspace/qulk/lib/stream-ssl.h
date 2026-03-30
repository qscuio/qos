#ifndef __HAVE_STREAM_SSL_H__
#define __HAVE_STREAM_SSL_H__

#include <stdbool.h>

bool stream_ssl_is_configured(void);
void stream_ssl_set_private_key_file(const char *file_name);
void stream_ssl_set_certificate_file(const char *file_name);
void stream_ssl_set_ca_cert_file(const char *file_name, bool bootstrap);
void stream_ssl_set_peer_ca_cert_file(const char *file_name);
void stream_ssl_set_key_and_cert(const char *private_key_file,
                                 const char *certificate_file);
void stream_ssl_set_protocols(const char *arg);
void stream_ssl_set_ciphers(const char *arg);

#define SSL_OPTION_ENUMS \
        OPT_SSL_PROTOCOLS, \
        OPT_SSL_CIPHERS

#define STREAM_SSL_LONG_OPTIONS                     \
        {"private-key", required_argument, NULL, 'p'}, \
        {"certificate", required_argument, NULL, 'c'}, \
        {"ca-cert",     required_argument, NULL, 'C'}, \
        {"ssl-protocols", required_argument, NULL, OPT_SSL_PROTOCOLS}, \
        {"ssl-ciphers", required_argument, NULL, OPT_SSL_CIPHERS}

#define STREAM_SSL_OPTION_HANDLERS                      \
        case 'p':                                       \
            stream_ssl_set_private_key_file(optarg);    \
            break;                                      \
                                                        \
        case 'c':                                       \
            stream_ssl_set_certificate_file(optarg);    \
            break;                                      \
                                                        \
        case 'C':                                       \
            stream_ssl_set_ca_cert_file(optarg, false); \
            break;                                      \
                                                        \
        case OPT_SSL_PROTOCOLS:                         \
            stream_ssl_set_protocols(optarg);           \
            break;                                      \
                                                        \
        case OPT_SSL_CIPHERS:                           \
            stream_ssl_set_ciphers(optarg);             \
            break;

#define STREAM_SSL_CASES \
    case 'p': case 'c': case 'C': case OPT_SSL_PROTOCOLS: case OPT_SSL_CIPHERS:

#endif /* __HAVE_STREAM_SSL_H__ */
