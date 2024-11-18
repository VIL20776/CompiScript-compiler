FROM fedora:40

RUN dnf update -y && dnf install -y cmake gcc-g++ antlr4 antlr4-cpp-runtime-devel libuuid-devel

WORKDIR /app
COPY . .

RUN cmake -S . -B build
RUN cmake --build build

CMD [ "bash" ]

