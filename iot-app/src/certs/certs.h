
#ifndef HOMETHINGS_CERTS_H_
#define HOMETHINGS_CERTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_CA_CERT_TAG 1
#define APP_DEV_CERT_TAG 2
#define APP_DEV_KEY_TAG 3

static const unsigned char dev_key_pem[] = {
#include "device-key.pem.h"
};

static const unsigned char dev_cert_pem[] = {
#include "device-cert.pem.h"
};

static const unsigned char ca_cert_pem[] = {
#include "ca-cert.pem.h"
};

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_CERTS_H_