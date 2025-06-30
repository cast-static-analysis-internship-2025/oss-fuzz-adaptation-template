FROM tuner-oss-fuzz-base

RUN dnf update -y && \
    dnf install -y gcc gcc-c++ unzip make git libtool pkg-config && \
    dnf clean all

WORKDIR /app/

RUN git clone --recursive --depth 1 --branch 1.25.5 git://git.ghostscript.com/mupdf.git mupdf

COPY pdf_fuzzer.cc build.sh corpus.tar.gz ./
