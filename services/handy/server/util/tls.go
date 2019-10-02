package util

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"flag"
	"fmt"
	"math/big"
	"net"
	"os"
	"time"
)

var (
	defaultRsaBits = flag.Int("tls-rsa-bits", 2048, "Size of RSA key to generate for TLS.")
	validFor       = flag.Duration("tls-valid-for", 30*24*time.Hour, "Duration of the self-signed certificate validity")
)

func createTLSConfig(certPath, keyPath, host string) error {
	priv, err := rsa.GenerateKey(rand.Reader, *defaultRsaBits)
	if err != nil {
		return fmt.Errorf("failed to generate private key: %s", err)
	}

	notBefore := time.Now()
	notAfter := notBefore.Add(*validFor)

	serialNumberLimit := new(big.Int).Lsh(big.NewInt(1), 128)
	serialNumber, err := rand.Int(rand.Reader, serialNumberLimit)
	if err != nil {
		return fmt.Errorf("failed to generate serial number: %s", err)
	}

	template := x509.Certificate{
		SerialNumber: serialNumber,
		Subject: pkix.Name{
			Country:      []string{"AE"},
			Organization: []string{"ProCTF 2019"},
		},
		NotBefore: notBefore,
		NotAfter:  notAfter,

		KeyUsage:              x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature | x509.KeyUsageCertSign,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		BasicConstraintsValid: true,
		IsCA:                  true,
	}

	if ip := net.ParseIP(host); ip != nil {
		template.IPAddresses = append(template.IPAddresses, ip)
	} else {
		template.DNSNames = append(template.DNSNames, host)
	}

	derBytes, err := x509.CreateCertificate(rand.Reader, &template, &template, &priv.PublicKey, priv)
	if err != nil {
		return fmt.Errorf("failed to create certificate: %s", err)
	}

	certOut, err := os.Create(certPath)
	defer certOut.Close()
	if err != nil {
		return fmt.Errorf("failed to open %s for writing: %s", certPath, err)
	}
	if err := pem.Encode(certOut, &pem.Block{Type: "CERTIFICATE", Bytes: derBytes}); err != nil {
		return fmt.Errorf("failed to write data to %s: %s", certPath, err)
	}

	keyOut, err := os.OpenFile(keyPath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0600)
	defer keyOut.Close()
	if err != nil {
		return fmt.Errorf("Failed to open %s for writing: %s", keyPath, err)
	}
	privBytes, err := x509.MarshalPKCS8PrivateKey(priv)
	if err != nil {
		return fmt.Errorf("Unable to marshal private key: %v", err)
	}
	if err := pem.Encode(keyOut, &pem.Block{Type: "PRIVATE KEY", Bytes: privBytes}); err != nil {
		return fmt.Errorf("Failed to write data to %s: %s", keyPath, err)
	}
	return nil
}

func CheckTLSConfig(certPath, keyPath, host string) error {
	certExists := false
	if _, err := os.Stat(certPath); err == nil {
		certExists = true
	} else if !os.IsNotExist(err) {
		return err
	}

	keyExists := false
	if _, err := os.Stat(keyPath); err == nil {
		keyExists = true
	} else if !os.IsNotExist(err) {
		return err
	}

	if certExists != keyExists {
		return fmt.Errorf("only one of TLS cert and key is present. Cert: %v, key: %v", certExists, keyExists)
	}
	if certExists && keyExists {
		return nil
	}

	return createTLSConfig(certPath, keyPath, host)
}
