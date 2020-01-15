# esp32 ota https升级记录
esp32 ota https升级为单向认证
## 生成证书
使用指令
```shell
openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 3650 -nodes
```
生成key文件ca_key.pem  
和证书文件ca_cert.pem  
将ca_cert.pem拷贝到代码中，ota升级代码中使用到的证书就是这个
## 服务器配置
以ubuntu为例，同时已经安装好apache2可以使用http访问
使用
```shell
a2enmod ssl
```
开启ssl模块
上面命令相当于
```javascript
sudo ln -s /etc/apache2/mods-available/ssl.load /etc/apache2/mods-enabled
sudo ln -s /etc/apache2/mods-available/ssl.conf /etc/apache2/mods-enabled
```
如果没有a2enmod指令，也可直接在apache2.conf中设置SSL模块加载
```shell
LoadModule ssl_module /usr/lib/apache2/modules/mod_ssl.so
```
修改apache配置文件
```shell
ln -s /etc/apache2/sites-available/default-ssl.conf /etc/apache2/sites-enabled/default-ssl.conf
vim /etc/apache2/sites-enabled/default-ssl.conf
```
修改文件中的
```shell
SSLEngine On
SSLOptions +StrictRequire
SSLCertificateFile /etc/ssl/certs/ca_cert.pem
SSLCertificateKeyFile /etc/ssl/private/ca_key.pem
```
重启Apache即可
```shell
/etc/init.d/apache2 restart
```
重启后即可使用https访问
## 测试说明
如果要让测试不通过，将ota升级代码中  
ca_cert.pem中的内容多修改一点，如果只修改一两个字符测试有可能还是会通过
## 参考链接
[https://yq.aliyun.com/articles/645049?spm=a2c4e.11153940.0.0.f86e2774jbe9uD](https://yq.aliyun.com/articles/645049?spm=a2c4e.11153940.0.0.f86e2774jbe9uD)  
[https://github.com/espressif/esp-idf/tree/master/examples/system/ota#run-https-server](https://github.com/espressif/esp-idf/tree/master/examples/system/ota#run-https-server)
