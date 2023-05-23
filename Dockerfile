FROM ubuntu:16.04

# 使用本地的mirror文件替换/etc/apt/sources.list
COPY ./mirror /etc/apt/sources.list

# 添加32位架构
RUN dpkg --add-architecture i386

# 更新并安装所需的库和工具
RUN apt-get update && apt-get install -y --no-install-recommends \
    python \
    python-pip \
    lame \
    lib32stdc++6 \
    lib32z1 \
    lib32ncurses5 \
    libbz2-1.0:i386 \
    && rm -rf /var/lib/apt/lists/*  # 清理缓存

# 使用pip安装Python库
RUN pip install setuptools==20.7.0
RUN pip install pyserial==3.5 pyyaml==5.4 requests==2.7.0 smmap==3.0.5 scons==3.1.2 yoctools==2.0.26 -i https://mirrors.aliyun.com/pypi/simple

# 复制本地的csky-elfabiv2-tools-x86_64-minilibc-20230301到镜像的指定位置
COPY ./csky-elfabiv2-tools-x86_64-minilibc-20230301 /opt/csky-elfabiv2-tools-x86_64-minilibc-20230301

# 添加到PATH环境变量中
ENV PATH="/opt/csky-elfabiv2-tools-x86_64-minilibc-20230301/bin:${PATH}"
