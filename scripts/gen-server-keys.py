
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

def create_ca_subject(domain:str):
    return [
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"UA"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"LWO"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u"Lviv"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Ostapenko CA"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"HomeThings CA"),
        x509.NameAttribute(NameOID.COMMON_NAME, domain),
    ]

def create_mqtt_subject(domain:str):
    return [
        x509.NameAttribute(NameOID.COUNTRY_NAME, u"UA"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"LWO"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u"Lviv"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Ostapenko"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"HomeThings"),
        x509.NameAttribute(NameOID.COMMON_NAME, domain),
    ]

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--ca-key", nargs="?", default=f"{KEYS_DIR}/homethings-ca-key.pem",
                        help="Store path to CA private key *.pem")

    parser.add_argument("--ca-cert", nargs="?", default=f"{KEYS_DIR}/homethings-ca-cert.crt",
                        help="Store path to CA cert key *.crt")

    parser.add_argument("--mqtt-key", nargs="?", default=f"{KEYS_DIR}/homethings-mqtt-key.pem",
                        help="Store path to MQTT private key *.pem")

    parser.add_argument("--mqtt-cert", nargs="?", default=f"{KEYS_DIR}/homethings-mqtt-cert.crt",
                        help="Store path to MQTT cert key *.crt")

    parser.add_argument("--mqtt-domain", nargs="?", default="homethings.io",
                        help="Domain Name")
    
    parser.add_argument("--client-key", nargs="?", default=f"{KEYS_DIR}/homethings-client-key.pem",
                        help="Store path to Client private key *.pem")
    
    parser.add_argument("--client-cert", nargs="?", default=f"{KEYS_DIR}/homethings-client-cert.crt",
                        help="Store path to Client cert key *.crt")

    parser.add_argument("--passphrase", nargs="?", default="",
                        help="Passphrase")

    args = parser.parse_args()

    # Generate CA
    ca_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )

    with open(args.ca_key, "wb") as file:
        file.write(ca_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))

    subject = issuer = x509.Name(create_ca_subject(args.mqtt_domain))

    ca_cert = x509.CertificateBuilder().subject_name(
        subject
    ).issuer_name(
        issuer
    ).public_key(
        ca_key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.datetime.utcnow()
    ).not_valid_after(
        datetime.datetime.utcnow() + datetime.timedelta(weeks=1000)
    ).add_extension(
        x509.SubjectKeyIdentifier.from_public_key(ca_key.public_key()),
        critical=False,
    ).add_extension(
        x509.BasicConstraints(True, None),
        critical=True,
    ).add_extension(
        x509.KeyUsage(digital_signature=False,
                      content_commitment=False,
                      key_encipherment=False,
                      data_encipherment=False,
                      key_agreement=False,
                      key_cert_sign=True,
                      crl_sign=True,
                      encipher_only=False,
                      decipher_only=False),
        critical=True,
    ).sign(ca_key, hashes.SHA256())

    with open(args.ca_cert, "wb") as file:
        file.write(ca_cert.public_bytes(serialization.Encoding.PEM))
   
    # Generate MQTT
    
    mqtt_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )
    
    with open(args.mqtt_key, "wb") as file:
        file.write(mqtt_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))
    
    mqtt_csr = x509.CertificateSigningRequestBuilder().subject_name(x509.Name(
        create_mqtt_subject(args.mqtt_domain)
    )).sign(mqtt_key, hashes.SHA256())
    
    mqtt_cert = x509.CertificateBuilder().subject_name(
        mqtt_csr.subject
    ).issuer_name(
        ca_cert.issuer
    ).public_key(
        mqtt_csr.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.datetime.utcnow()
    ).not_valid_after(
        datetime.datetime.utcnow() + datetime.timedelta(weeks=1000)
    ).sign(ca_key, hashes.SHA256())
    
    with open(args.mqtt_cert, "wb") as file:
        file.write(mqtt_cert.public_bytes(serialization.Encoding.PEM))
    
    # Generate client
    
    client_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )
    
    with open(args.client_key, "wb") as file:
        file.write(client_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))
    
    client_csr = x509.CertificateSigningRequestBuilder().subject_name(x509.Name(
        create_mqtt_subject("client")
    )).sign(client_key, hashes.SHA256())
    
    client_cert = x509.CertificateBuilder().subject_name(
        client_csr.subject
    ).issuer_name(
        ca_cert.issuer
    ).public_key(
        client_csr.public_key()
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
        x509.SubjectKeyIdentifier.from_public_key(client_csr.public_key()),
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
    
    with open(args.client_cert, "wb") as file:
        file.write(client_cert.public_bytes(serialization.Encoding.PEM))
