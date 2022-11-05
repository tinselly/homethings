
import os
import argparse
import datetime
import ipaddress

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
from cryptography import x509

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
ROOT_DIR = f"{BASE_DIR}/../"
KEYS_DIR = f"{ROOT_DIR}/certs/"
SRC_DIR = f"{ROOT_DIR}/iot-app/src/certs/"

def create_device_subject(domain:str):
    return [
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"UA"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"LWO"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u"Lviv"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Ostapenko"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"HomeThings"),
        x509.NameAttribute(NameOID.COMMON_NAME, domain),
    ]

def pem_to_c_string(pem:bytes) -> bytes:
    lines = pem.split(b"\n")
    
    result = b""
    
    for line in lines:
        if len(line) != 0:
            result += b"\"" + line + b"\\r\\n\"\n"
    
    return result

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--ca-key", nargs="?", default=f"{KEYS_DIR}/homethings-ca-key.pem",
                        help="Store path to CA private key *.pem")

    parser.add_argument("--ca-cert", nargs="?", default=f"{KEYS_DIR}/homethings-ca-cert.crt",
                        help="Store path to CA cert *.crt")
    
    parser.add_argument("--device-key", nargs="?", default=f"{SRC_DIR}/device-key.pem",
                        help="Store path to Device private key *.pem")
    
    parser.add_argument("--device-cert", nargs="?", default=f"{SRC_DIR}/device-cert.pem",
                        help="Store path to Device cert  *.pem")

    parser.add_argument("--device-ca-cert", nargs="?", default=f"{SRC_DIR}/ca-cert.pem",
                        help="Store path to CA cert *.pem")

    parser.add_argument("--passphrase", nargs="?", default="",
                        help="Passphrase")

    args = parser.parse_args()

    ca_key = ""
    ca_cert = ""
    with open(args.ca_key, "rb") as file:
        ca_key = serialization.load_pem_private_key(file.read(), None)

    with open(args.ca_cert, "rb") as file:
        ca_cert = x509.load_pem_x509_certificate(file.read())
    
    # Generate device
    
    ca_cert_bytes = ca_cert.public_bytes(serialization.Encoding.PEM)
    with open(args.device_ca_cert, "wb") as file:
        file.write(ca_cert_bytes)
    with open(f"{args.device_ca_cert}.h", "wb") as file:
        file.write(pem_to_c_string(ca_cert_bytes))
    
    device_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )
    
    device_key_bytes = device_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )

    with open(args.device_key, "wb") as file:
        file.write(device_key_bytes)
    with open(f"{args.device_key}.h", "wb") as file:
        file.write(pem_to_c_string(device_key_bytes))
  
    device_csr = x509.CertificateSigningRequestBuilder().subject_name(x509.Name(
        create_device_subject("device")
    )).sign(device_key, hashes.SHA256())
    
    device_cert = x509.CertificateBuilder().subject_name(
        device_csr.subject
    ).issuer_name(
        ca_cert.issuer
    ).public_key(
        device_csr.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.datetime.utcnow()
    ).not_valid_after(
        datetime.datetime.utcnow() + datetime.timedelta(weeks=1000)
    ).add_extension(
        x509.BasicConstraints(False, None),
        critical=False,
    ).add_extension(
        x509.SubjectKeyIdentifier.from_public_key(device_csr.public_key()),
        critical=False,
    ).add_extension(
        x509.AuthorityKeyIdentifier.from_issuer_public_key(
            ca_cert.public_key()),
        critical=False,
    ).add_extension(
        x509.KeyUsage(digital_signature=True,
                      content_commitment=True,
                      key_encipherment=True,
                      data_encipherment=False,
                      key_agreement=False,
                      key_cert_sign=False,
                      crl_sign=False,
                      encipher_only=False,
                      decipher_only=False),
        critical=True,
    ).add_extension(
        x509.ExtendedKeyUsage(
            [x509.ExtendedKeyUsageOID.CLIENT_AUTH,
             x509.ExtendedKeyUsageOID.EMAIL_PROTECTION]),
        critical=False,
    ).sign(ca_key, hashes.SHA256())
    
    
    device_cert_bytes = device_cert.public_bytes(serialization.Encoding.PEM)

    with open(args.device_cert, "wb") as file:
        file.write(device_cert_bytes)
    with open(f"{args.device_cert}.h", "wb") as file:
        file.write(pem_to_c_string(device_cert_bytes))

