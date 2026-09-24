Mbed TLS
=====================

:link_to_translation:`en:[English]`

Overview 
------------------------------

MbedTLS is a crypto algorithm library for embedded systems, which mainly implements basic crypto algorithms, X.509 certificate operations, SSL/TLS and DTLS protocols.

MbedTLS version
------------------------------

the currently supported version：3.5.2

MbedTLS introduction
------------------------------

`API Reference <mbed-tls.readthedocs.io/projects/api/en/v3.5.2/>`_

`KnowledgeBase <mbed-tls.readthedocs.io/en/latest/kb/>`_


TE200（TrustEngine-200）Introduction
--------------------------------------

BK7258 uses TE200 (TrustEngine-200) as the hardware acceleration engine, providing the following security features:

 - High security assurance. TE200 supports key hierarchy, lifecycle management, and true random number generator (TRNG) to enhance system security.
 - High-performance and low-power encryption/decryption operations. This is achieved through the internal cryptographic engine of TE200.
 - Reduced software complexity in security aspects. TE200 implements some security functions, such as lifecycle management and access control to OTP1.

TE200 Features 
+++++++++++++++++++++++++++++

TE200 supports the following features:

 - Symmetric encryption algorithms:：AES-ECB/CBC/CTR/CBC-MAC/CMAC/CCM/GCM（密钥长度为128位、192位和256位）
 - Symmetric encryption algorithms:：SM4-ECB/CBC/CTR/CBC-MAC/CMAC/CCM/GCM
 - Hash algorithms：SHA1/224/256
 - Hash algorithms：SM3
 - Asymmetric encryption algorithms：RSA 1024/2048/3072/4096 和 ECCP 192/224/256/384/512/521
 - Asymmetric encryption algorithms：SM2
 - Key ladder for key management
 - Life cycle management
 - True random number generator.
 - One Time Programming (OTP).

Supported Standards and Specifications
-------------------------------------------------

TrustEngine is compliant with the following standards:

  - FIPS PUB 180-4: Secure Hash Standard (SHS)
  - FIPS PUB 197: Advanced Encryption Standard (AES)
  - NIST SP 800-38A: Recommended Methods and Techniques for Block Cipher Modes of Operation
  - NIST SP 800-38B: Block Cipher Mode of Operation Protocol - CMAC Mode for Authentication
  - NIST SP 800-38C: Block Cipher Mode of Operation Protocol - CCM Mode for Authentication and Privacy
  - NIST SP 800-38D: Block Cipher Operating Mode Protocols - Galois/Counter Mode (GCM) and GMAC
  - NIST SP 800-90B: Entropy Source Protocol for Random Bit Generation
  - GB/T 32918-2016: SM2 Elliptic Curve Public Key Cryptography Algorithm
  - GB/T 32905-2016: SM3 Password Hashing Algorithm
  - GB/T 32907-2016: SM4 block cipher algorithm

Among them: SM2 public key cryptography, SM3 cryptographic hash algorithm and SM4 block cipher algorithm are Chinese
national standards.

Related Macro Explanation
--------------------------------

.. code::

 - CONFIG_MBEDTLS                      #MbedTLS historical version V2.28.5, this macro is disabled by default
 - CONFIG_FULL_MBEDTLS                 #MbedTLS pruning, disabling it can optimize codesize
 - CONFIG_PSA_MBEDTLS                  #Supports MbedTLS V3.5.2
 - CONFIG_MBEDTLS_USE_PSA_CRYPTO       #TLS protocol supports PSA API
 - CONFIG_TRUSTENGINE                  #Supports hardware acceleration, enabling it will increase codesize
 - CONFIG_SW_CRYPTO                    #Supports pure software implementation, mutually exclusive with CONFIG_TRUSTENGINE
 - CONFIG_TE200_ENTROPY                #Supports TrustEngine random number entropy source
 - CONFIG_MBEDTLS_LOG_LEVEL="OFF"      #Sets MbedTLS debug log level (OFF/ERROR/WARNING/INFO/DEBUG)
 - CONFIG_PSA_MBEDTLS_TEST             #Supports PSA API testing
 - CONFIG_TRUSTENGINE_TEST             #Supports MbedTLS API and performance testing

project Default Configuration
++++++++++++++++++++++++++++++++++++++

.. code::

    CONFIG_MBEDTLS=n
    CONFIG_FULL_MBEDTLS=n
    CONFIG_PSA_MBEDTLS=y
    CONFIG_MBEDTLS_USE_PSA_CRYPTO=n
    CONFIG_MBEDTLS_ACCELERATOR=n
    CONFIG_TRUSTENGINE=n
    CONFIG_SW_CRYPTO=y
    CONFIG_TE200_ENTROPY=y
    CONFIG_MBEDTLS_LOG_LEVEL="OFF"
    CONFIG_PSA_MBEDTLS_TEST=n
    CONFIG_TRUSTENGINE_TEST=n

Hardware Acceleration Configuration
++++++++++++++++++++++++++++++++++++++

.. code::

    CONFIG_TRUSTENGINE=y
    CONFIG_SW_CRYPTO=n

.. important::
    *Note：TrustEngine has adapted MbedTLS API, and the upper-layer application API calls remain unchanged after hardware acceleration is enabled; 
    Enabling hardware acceleration will increase code size;
    TrustEngine can only run on a single CPU*

MbedTLS Algorithm Configuration File
++++++++++++++++++++++++++++++++++++++

.. code::

  components\psa_mbedtls\mbedtls_port\configs\mbedtls_psa_crypto_config.h