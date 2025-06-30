# OSS-Fuzz Project Adaptation Task

This task is inspired by a real challenge we faced in our work.
We're experimenting with how different [GCC optimization flags](https://gcc.gnu.org/onlinedocs/gcc-3.3.4/gcc/Optimize-Options.html) affect the runtime performance of C/C++ programs.
To do this effectively, we need a collection of diverse real-world C/C++ projects that can be automatically built and executed on fixed input data.
This allows us to compile the projects using different optimization flags and measure their runtime under each flag sequence.
For those experiments to work, it's important that each project has stable runtime and that its execution covers a significant portion of the code.

To make the process of collecting such projects more efficient, we decided to use open-source projects from [OSS-Fuzz](https://github.com/google/oss-fuzz).
These projects are originally designed for fuzz testing,
but we adapt them so they can be compiled without any fuzzing-related options and instead with any compiler flags we want to test.

Your task is to modify a given OSS-Fuzz project so that it fits these needs.
A more detailed description of the task is provided in the sections below:

* Chapter 1: Overview of OSS-Fuzz and its main components
* Chapter 2: Project adaptation steps
* Chapter 3: Example project
* Chapter 4: Testing your solution
* Chapter 5: Conclusion

Successfully adapting the projects requires knowledge in several areas.
Be sure to review the topics below before starting the assignment:

* C and C++ programming languages
* Working in a Linux environment
* Bash scripting
* Git and GitHub for version control
* The compilation process using GCC
* Build systems such as Make and CMake
* Fuzzing, particularly AFL++
* Python programming language
* Docker

## 0. Working Environment
We primarily work in a Linux environment, specifically Ubuntu.
All command examples in this assignment are written for the Linux terminal,
so it's recommended that you use or have access to a Linux system.

## 1. OSS-Fuzz Usage
OSS-Fuzz is an open-source fuzzing service developed by Google
to improve the security and reliability of open-source software.
It automatically tests projects for bugs and vulnerabilities using fuzzing.
It includes more than 500 C/C++ projects with their fuzzers.

We'll be using projects from OSS-Fuzz for adaptation,
but we first need to understand how OSS-Fuzz operates.
This chapter provides a general guide on its intended usage.
You can find any additional information about OSS-Fuzz in [this GitHub repository](https://github.com/google/oss-fuzz).

### 1.1. Clone OSS-Fuzz
First, clone OSS-Fuzz repository:
```shell
git clone https://github.com/google/oss-fuzz.git
```

### 1.2. Download Required Images
Download all the Docker images that OSS-Fuzz may need in the future
by executing the following command in the root directory of the OSS-Fuzz project:
```shell
python3 infra/helper.py pull_images
```

### 1.3. Project Navigation
To view the available projects in OSS-Fuzz, check the `oss-fuzz/projects` directory.
Each project in OSS-Fuzz contains a `project.yaml` file, which provides details about the project,
including the programming language, main repository, and supported fuzzing engines.

For instance, in `oss-fuzz/projects/mupdf/project.yaml`, you will find:
```yaml
main_repo: git://git.ghostscript.com/mupdf.git
language: c++
fuzzing_engines:
  - libfuzzer
  - afl
  - honggfuzz
```

### 1.4. Building Fuzzers
Next, build the AFL++ fuzzer for a specific project using:
```shell
python3 infra/helper.py build_fuzzers --engine afl <PROJECT>
```
Here, `<PROJECT>` refers to any project within the `oss-fuzz/projects/` directory that supports AFL++ fuzzing engine.
After executing this command, built fuzzers will be available in the `oss-fuzz/build/out/<PROJECT>` directory.
A fuzzer is an executable (usually has a filename ending in `_fuzzer`) that automatically feeds random inputs
into the program to uncover bugs, crashes, or security vulnerabilities.

***NOTE:*** Fuzzers can be built using different fuzzing engines, but we primarily use AFL++.

### 1.5. Running Fuzzers
After building the fuzzer, you can run it using the following command:
```shell
python3 infra/helper.py run_fuzzer --engine afl <PROJECT> <FUZZER_NAME>
```
Each project may contain multiple fuzzers designed to test different parts of the code.
To run a fuzzer, you need to specify the desired fuzzer by providing its name as `<FUZZER_NAME>`.
After building the project, you can find the available fuzzers in the `oss-fuzz/build/out/<PROJECT>` directory.

Running an AFL++ fuzzer will automatically generate an input corpus in the output directory of the project.
The corpus is a directory containing multiple files, each serving as an input sample for the project.

### 1.6. AFL++ Flags
When running an AFL++ fuzzer you can additionally provide arguments to AFL++:
```shell
python3 infra/helper.py run_fuzzer --engine afl <PROJECT> <FUZZER_NAME> -- <AFL_FLAGS>
```
Here are a couple of useful flags in AFL++:
```
-V seconds    - fuzz for a specified time then terminate
-g minlength  - set min length of generated fuzz input (default: 1)                                                                                         
-G maxlength  - set max length of generated fuzz input (default: 1048576)                                                                                   
```
Here is an example of the run command with AFL++ flags:
```shell
python3 infra/helper.py run_fuzzer --engine afl fftw3 fftw3_fuzzer -- -V 10 -g 1000
```

## 2. Adapting Project
Fuzzers cannot be directly used for our goal,
as they run on randomly generated input data, resulting in unstable runtimes.
However, OSS-Fuzz projects still provide an excellent foundation because they come with pre-configured build environment.
**By removing the fuzzing components from these projects and running them on a fixed corpus,
we can achieve stable execution, making them suitable for use.**
Additionally, by using their fuzzing capabilities,
we can generate an input corpus with large coverage for the target project.

Adapting OSS-Fuzz projects requires certain modifications.
The first step is to copy the project's contents into your fork of this repository.
```shell
cp -r /oss-fuzz/projects/<PROJECT> /OSS-Fuzz-Project-Adaptation/
```
And make all the modifications in `/OSS-Fuzz-Project-Adaptation/<PROJECT>` directory.
Replace `<PROJECT>` with the name of the project you have been given.

The main goal is to establish a custom build environment for the OSS-Fuzz project.
The process involves two key phases:
1. **Corpus Generation:** Create a "good" input corpus for the selected project.
2. **Fuzzing Elimination:** Customize the build configuration to enable direct execution of the project
   against the input corpus, eliminating the fuzzing operations.

### 2.1. Creating Corpus
A corpus is a directory containing files whose content can be used as input for your project.
You have to create such a directory for your chosen project.
Corpus generation is a crucial step as it determines project execution time and coverage.
The corpus should be neither too small nor too large.
Ideally, its size should be such that the program takes between 5 and 15 seconds to process it.

The corpus can be created either manually or automatically using the AFL++ fuzzer.
An effective corpus includes both manually selected and automatically generated input files.

#### 2.1.1. Automatically Generated Corpus
When AFL++ runs on a project, it automatically generates and stores the corpus in
`oss-fuzz/build/out/<PROJECT>/<FUZZER_NAME>_afl_address_out/default/queue` directory.
Input files are named starting with `id:`.
Everything else in this directory can be deleted, as it won't be needed.

During execution, the fuzzer provides coverage metrics for the corpus.
The longer the fuzzer runs, the larger the generated corpus becomes.

Randomly generated corpus might not perform well if the program expects a specifically formated data.
It is important to note that some projects include initial configurations and dictionaries for their fuzzer,
which can improve performance.
You can also experiment with adding your own such configurations to enhance the effectiveness of the fuzzers.

#### 2.1.2. Manually Selected Corpus
In some cases, corpus files can be selected manually.
For example, by analyzing the
[fuzzer entry point of the Unrar project](https://github.com/google/oss-fuzz/blob/master/projects/unrar/unrar_fuzzer.cc),
we can see that input files are interpreted as `.rar` files.

Therefore, adding carefully chosen diverse RAR files to the corpus
can be beneficial for improving project coverage.

#### 2.1.3 Corpus Location
After collecting a valid corpus in `OSS-Fuzz-Project-Adaptation/<PROJECT>/corpus` directory,
it should be archived and saved in the project directory as `OSS-Fuzz-Project-Adaptation/<PROJECT>/corpus.tar.gz`:
```shell
tar -czf /OSS-Fuzz-Project-Adaptation/<PROJECT>/corpus.tar.gz -C /OSS-Fuzz-Project-Adaptation/<PROJECT>/ corpus
```

### 2.2. Fuzzing Elimination
To run the project on a fixed input corpus, the fuzzer must be removed from the project.
This requires modifying both the source code and the build system to remove dependencies on the fuzzer.

#### 2.2.1. Build Environment Overview
Fuzzers are built in a docker environment which is defined in a `oss-fuzz/projects/<PROJECT>/Dockerfile`.
In that environment OSS-Fuzz runs `build.sh` script which are usually located in `oss-fuzz/projects/<PROJECT>/build.sh`.
But in some cases that `build.sh` script might be missing and is actually downloaded in a `Dockerfile`.

The main point of the `build.sh` script is to build fuzzers in `$OUT` directory,
which is set to `oss-fuzz/build/out/<PROJECT>/`.
Build scripts also depend on variables such as `$CXX`, `$CC`, `$CXXFLAGS`, `$CFLAGS`, `$WORK`, `$SRC`, etc.

#### 2.2.2. Source Code Modifications
If we want to build the project without a fuzzer, modifications to the source code are usually required.
Projects that use the `LLVMFuzzerTestOneInput` function do not include a `main` function,
as they rely on the `-fsanitize=fuzzer` flag when compiled with clang.
Additionally, we need to execute `LLVMFuzzerTestOneInput` on all files in the collected input corpus.
To achieve this, we can add a `main` function that iterates over the corpus files
and passes their contents to `LLVMFuzzerTestOneInput`.

For example, you can add the following code to the project's fuzzer source file:
```cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

void processFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!buffer.empty()) {
        LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <corpus_directory>" << std::endl;
        return 1;
    }

    fs::path corpusDir = argv[1];

    if (!fs::exists(corpusDir) || !fs::is_directory(corpusDir)) {
        std::cerr << "Invalid corpus directory: " << corpusDir << std::endl;
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(corpusDir)) {
        if (fs::is_regular_file(entry)) {
            processFile(entry.path());
        }
    }

    return 0;
}
```

#### 2.2.3. Build Script Modifications
The project's `build.sh` script typically compiles the project
with sanitizer options and links it to fuzzer libraries.
These must be removed, as we need the project to compile
with various optimization flags to achieve the best possible performance.

Each project's `build.sh` script is uniquely written, but after modification,
all scripts should follow a consistent interface.
They should rely only on four environmental variables:

* `$TUNER_COMPILER_BIN` – Path to the **GCC 10.3.0** compiler bin directory.
* `$TUNER_FLAGS` – Compiler flags to be applied for C and C++ projects.
* `$TUNER_OUTPUT` – Output directory where the compiled binary should be placed.
* `$TUNER_CORE_COUNT` - Number of cores that can be used by the script.

For example, after your changes, you should be able to run the build script like this:
```shell
TUNER_COMPILER_BIN="/usr/bin/" TUNER_CORE_COUNT=4 TUNER_OUTPUT="/path/to/out/" TUNER_FLAGS="-O3" ./build.sh
```

After execution, the `$TUNER_OUTPUT` directory should contain a `tune_me` binary,
which expects a single argument: the path to the corpus directory.

```shell
./tune_me /path/to/corpus
```

The `build.sh` script should be executable from any directory without affecting the build process.
And most importantly, the script must be designed for parallel execution
with different flags and output directories, ensuring that concurrent runs do not interfere with each other.
For example, you should be able to run the following two commands simultaneously
without their build processes interfering with each other:
* `... TUNER_OUTPUT="./out1/" ./build.sh`
* `... TUNER_OUTPUT="./out2/" ./build.sh`

#### 2.2.4. Dockerfile Modifications
Projects in OSS-Fuzz are built within a Docker environment to handle varying dependencies across different projects.
A Dockerfile is used to specify these dependencies.
However, some modifications to those Dockerfiles are needed.
The `Dockerfile` for the project is located at `oss-fuzz/projects/<PROJECT>/Dockerfile`.

As you can see from the `Dockerfile`,
OSS-Fuzz depends on its own base Docker image:
```
FROM gcr.io/oss-fuzz-base/base-builder
```
Which we don't really need.

Instead, we'll use our own custom base image containing **GCC 10.3.0**.
First, we need to build [the base image](Dockerfile). Navigate to the project's root directory and run:
```shell
docker build -t tuner-oss-fuzz-base .
```
This may take some time...

Once built, update your project's `Dockerfile` to use this base image instead by setting:
```
FROM tuner-oss-fuzz-base
```

Secondly, when installing dependencies and packages, they use the `apt` package manager.
For example, you might encounter:
```
RUN apt-get update && apt-get install -y make libtool pkg-config
```

However, our base image uses the `dnf` package manager instead of `apt`.
Therefore, these dependencies should be installed using `dnf`.
You can replace the original command with:
```
RUN dnf update -y && \
    dnf install -y gcc gcc-c++ unzip make git libtool pkg-config && \
    dnf clean all
```

Thirdly, you'll notice that a project's source code is typically installed in a `Dockerfile` using Git.
For example, the `Dockerfile` might include:
```
RUN git clone --recursive --depth 1 git://git.ghostscript.com/mupdf.git mupdf
```
However, the repository's default branch may change over time.
To ensure consistency, it's best to specify a fixed version when cloning.
You can set it to the latest available version of the project with `--branch` argument (or you can use commit hash):
```
RUN git clone --recursive --depth 1 --branch 1.25.5 git://git.ghostscript.com/mupdf.git mupdf
```

Finally, OSS-Fuzz Dockerfiles rely on environment variables like `$SRC`,
which we won't be using, so they should be removed.
Instead, copy `build.sh`, `corpus.tar.gz`, and any other dependencies into the `/app` directory.

Generally, aside from the steps mentioned above, any steps in the build process
that only need to be run once before compiling the project should be included in the `Dockerfile`.

After modifications, you can build and run your container from the directory of your project.
```shell
docker build -t tuner-<PROJECT> .
docker run --rm --name <PROJECT>-test -it tuner-<PROJECT> /bin/bash
```
Inside a docker container GCC 10.3.0 is located in `/gcc/gcc-10.3.0-bin/bin/` directory.

## 3. Example Project
You can find an example project, MuPDF, along with the modifications made to it
[here](example/mupdf).

The project's directory contains four key components:
* The `build.sh` script, for building the project which depends on `TUNER_*` arguments.
* A `Dockerfile` that extends the `tuner-oss-fuzz-base` image and installs all dependencies for the MuPDF project.
* `corpus.tar.gz`, a pre-collected dataset for running MuPDF on a fixed corpus.
* `pdf_fuzzer.cc`, a modified source file that executes MuPDF on the fixed corpus.

## 4. Test Your Project
After making all necessary changes to the project, you can test it using the following steps.

### 4.1. Manual Testing
If you want to manually configure your build and run settings, here are the steps to follow.

#### 4.1.1. Build Docker Image
Replace `<PROJECT>` with your project name:
```shell
docker build -t tuner-<PROJECT> .
```

#### 4.1.2. Run Docker Container
Create and start a container from your image:
```shell
docker run --rm --name <PROJECT>-test -it tuner-<PROJECT> /bin/bash
```

#### 4.1.3. Test Build Script
```shell
mkdir /app/out/
TUNER_COMPILER_BIN=/gcc/gcc-10.3.0-bin/bin \
TUNER_CORE_COUNT=4 \
TUNER_OUTPUT=/app/out \
TUNER_FLAGS="-O3" \
/app/build.sh
```

#### 4.1.4. Run Generated Binary On Corpus
```shell
tar -xzf /app/corpus.tar.gz -C /app/
time /app/out/tune_me /app/corpus/
```
Ensure that the binary's execution time is stable and falls within the range of 5 to 15 seconds.

### 4.2. Automatic Testing
A script is available for automatically testing the build and run configurations of your project.
To use it, create and run a Docker container,
then execute the testing script located at `/test_project.sh` inside the container.
```shell
docker build -t tuner-<PROJECT> .
docker run --rm --name <PROJECT>-test -it tuner-<PROJECT> /bin/bash
/test_project.sh
```
The script will exit with a non-zero status if an error occurs.

## 5. Conclusion
Once you've made all the changes and tested everything to ensure it works correctly, create a pull request for us to review.
