FROM openeuler/openeuler:24.03

RUN sed -i '/^[[:space:]]*metalink=/{s/^/#/}' /etc/yum.repos.d/openEuler.repo
RUN dnf update -y && \
    dnf install -y gcc gcc-c++ vim make gmp-devel mpfr-devel libmpc-devel flex bison texinfo wget diffutils binutils diffutils python3-pip python3-virtualenv perf && \
    dnf clean all

COPY create_gcc_release.sh /gcc/
RUN /gcc/create_gcc_release.sh
COPY test_project.sh /
RUN dnf remove -y --noautoremove gcc
ENV PATH="/gcc/gcc-10.3.0-bin/bin:${PATH}"
