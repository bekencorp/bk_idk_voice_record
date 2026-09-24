Mbed TLS
=====================

:link_to_translation:`en:[English]`

概述
------------------------------

MbedTLS是一个适用于嵌入式系统的算法库，主要实现基础加密算法、X.509证书操作、SSL/TLS和DTLDS协议。

MbedTLS版本
------------------------------

当前支持版本为：3.5.2

MbedTLS介绍
------------------------------

`API Reference <mbed-tls.readthedocs.io/projects/api/en/v3.5.2/>`_

`KnowledgeBase <mbed-tls.readthedocs.io/en/latest/kb/>`_


TE200（TrustEngine-200）介绍
------------------------------

BK7258采用了TE200（TrustEngine-200）作为硬件加速引擎，提供下述安全特性：

 - 高度安全保障。TE200 支持密钥阶梯、生命周期管理和真随机数发生器（TRNG），以提升系统的安全性。
 - 高性能和低功耗的加密/解密操作。这是通过 TE200 内部的加密引擎实现的。
 - 减少安全方面的软件复杂性。TE200 实现了一些安全功能，如生命周期管理，对 OTP1 的访问控制，

TE200 功能
+++++++++++++++++++++++++++++

TE200 支持下述特性：

 - 对称加密算法：AES-ECB/CBC/CTR/CBC-MAC/CMAC/CCM/GCM（密钥长度为128位、192位和256位）。
 - 对称加密算法：SM4-ECB/CBC/CTR/CBC-MAC/CMAC/CCM/GCM。
 - 散列算法：SHA1/224/256。
 - 散列算法：SM3。
 - 非对称加密算法：RSA 1024/2048/3072/4096 和 ECCP 192/224/256/384/512/521。
 - 非对称加密算法：SM2。
 - 密钥阶梯用于密钥管理。
 - 生命周期管理。
 - 真随机数发生器。
 - 一次性编程（OTP）。

它符合以下标准：

 - FIPS PUB 180-4：安全散列标准（SHS）
 - FIPS PUB 197：高级加密标准（AES）
 - NIST SP 800-38A：块密码工作模式的推荐方法和技术
 - NIST SP 800-38B：块密码工作模式协议——用于认证的CMAC模式
 - NIST SP 800-38C：块密码工作模式协议——用于认证和保密的CCM模式
 - NIST SP 800-38D：块密码工作模式协议——Galois/Counter Mode（GCM）和GMAC
 - NIST SP 800-90B：用于随机比特生成的熵源协议
 - GB/T 32918-2016：SM2椭圆曲线公钥密码算法
 - GB/T 32905-2016：SM3密码散列算法
 - GB/T 32907-2016：SM4块密码算法

其中：SM2公钥密码、SM3密码散列算法和SM4块密码算法是中国国家标准。

相关宏说明
--------------------------------

.. code::

 - CONFIG_MBEDTLS                      #MbedTLS历史版本V2.28.5，该宏默认关闭
 - CONFIG_FULL_MBEDTLS                 #对MbedTLS进行裁剪，关闭后可优化codesize
 - CONFIG_PSA_MBEDTLS                  #支持MbedTLS V3.5.2
 - CONFIG_MBEDTLS_USE_PSA_CRYPTO       #tls协议支持PSA API
 - CONFIG_TRUSTENGINE                  #支持硬件加速，开启后codesize会增加
 - CONFIG_SW_CRYPTO                    #支持纯软件实现，与CONFIG_TRUSTENGINE互斥
 - CONFIG_TE200_ENTROPY                #支持TrustEngine随机数熵源
 - CONFIG_MBEDTLS_LOG_LEVEL="OFF"      #设置MbedTLS调试log等级（OFF/ERROR/WARNING/INFO/DEBUG）
 - CONFIG_PSA_MBEDTLS_TEST             #支持PSA API测试
 - CONFIG_TRUSTENGINE_TEST             #支持MbedTLS API及性能测试

project默认配置
++++++++++++++++++++++++++++++++

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

支持硬件加速配置
++++++++++++++++++++++++++++++++

通过以下宏配置即可开启硬件加速：

.. code::

    CONFIG_TRUSTENGINE=y
    CONFIG_SW_CRYPTO=n

.. important::
    *Note：TrustEngine已针对MbedTLS API做了适配，开启硬件加速后上层应用调用的mbedtls API不变;
    开启硬件加速后会增加codesize;
    TrustEngine只能运行在一个cpu上*

MbedTLS算法配置文件
++++++++++++++++++++++++++++++++

.. code::

  components\psa_mbedtls\mbedtls_port\configs\mbedtls_psa_crypto_config.h