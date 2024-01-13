# Getting Started

Now that you know that RTypeNetCommon is THE network library for you, it's time to get ready !

## Downloading the library

The library is available on GitHub, you can download it by cloning the repository:

```bash
git clone https://github.com/Vive-le-LEX/RTypeNetCommon.git
```

Or by using the cmake FetchContent module:

```cmake
FetchContent_Declare(
    RTypeNetCommon
    GIT_REPOSITORY https://github.com/Vive-le-LEX/RTypeNetCommon.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(RTypeNetCommon)
```

## Building the library

These steps are just here to inform you of the different ways of including the library in you project since it's header only.

### Downloading the library

You can either download the library using git or using the cmake FetchContent module.

#### Using git

```bash
git clone -b main --single-branch https://github.com/Vive-le-LEX/RTypeNetCommon.git
```

#### Using FetchContent

```cmake
FetchContent_Declare(
    RTypeNetCommon
    GIT_REPOSITORY https://github.com/Vive-le-LEX/RTypeNetCommon.git
    GIT_TAG main
)

FetchContent_MakeAvailable(RTypeNetCommon)
```

### Configuring the library

Nothing need to be configured since it's a header only library.

### Using the library

Juste include RTypeNet.hpp in your project.
And don't forget to build with it.
If you are using cmake add this line to your CMakelists.txt
```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE RTypeNetCommon)
```

<div class="section_buttons">
| Previous          |                              Next |
|:------------------|----------------------------------:|
| [Home](Welcome.md) | [TCP Tutorial](TCPModule.md) |
</div>
