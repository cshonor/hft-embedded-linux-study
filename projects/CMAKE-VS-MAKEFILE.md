# CMake 与 Makefile：什么关系？

> 构建工具 FAQ · 对齐 `projects/*/part-*/Makefile` 脚手架  
> [projects 总览](./README.md)

---

## 你的理解（对一半）

都跟**项目怎么编译/链接**有关；CMake 更像「升级版工作流」：

| | **Makefile** | **CMake** |
|---|--------------|-----------|
| 你写什么 | 手工写详细规则（目标、依赖、命令） | 写更统一的 `CMakeLists.txt` |
| 跨平台 | 常要按平台改规则 | 同一套配置，生成各平台构建文件 |
| 产物 | 直接驱动 `make` | 可生成 Makefile、Ninja、VS 工程等 |

Makefile：规则自己写死，平台差异容易踩坑。  
CMake：配置一次，按生成器产出对应平台的构建文件，跨平台更省事。

---

## 「CMake = Makefile 的插件？」——不能这么说

虽然 CMake **经常会生成** Makefile，但它是**独立的构建系统**，不是 make 的插件：

1. 有自己的语法与功能（`CMakeLists.txt`）  
2. 控制整条配置 → 生成 → 编译流程  
3. Makefile **只是众多后端之一**（还有 Ninja、Xcode、Visual Studio …）  

所以：CMake 比「插件」大得多、也复杂得多；make 可以单独用，也可以当 CMake 的生成目标。

---

## 对本仓库的含义

当前多数 `part-*` 脚手架用的是 **手写 Makefile**（简单、一眼看懂、WSL/Linux 直接 `make`）。  
项目变大、要 Windows/Linux/多编译器时，再考虑迁到 **CMake** 生成 Ninja/Make。

```text
手写 Makefile  ──直接──▶  make
CMakeLists.txt ──生成──▶  Makefile / build.ninja / .sln  ──再──▶  编译
```

---

## 一句话

> Makefile = 具体的「施工单」；CMake = 写施工说明并**按工地生成**施工单的系统。不是插件关系。
