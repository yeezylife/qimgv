<!-- 语言切换链接 -->
<div align="right">
  <a href="#zh">🇨🇳 中文</a> | <a href="#en">🇬🇧 English</a>
</div>
  <h1 align="center">qimgv —— 现代化高性能图片查看工具</h1>

  <p align="center">
    <strong>基于 Qt6 和 C++23 重构，流畅、高效、智能</strong>
  </p>

  <h2>📖 项目简介</h2>
  <p>该项目的代码经过全面重构，基于 Qt6 和 C++23 架构，采用最新的编程技术和最佳实践，提供了流畅、高效的图片浏览体验。</p>

  <h2>✨ 核心特性</h2>
  <ul>
    <li><strong>高性能渲染</strong>：利用 Qt6 的硬件加速和优化算法</li>
    <li><strong>现代化架构</strong>：基于 C++23 特性的模块化设计</li>
    <li><strong>智能内存管理</strong>：使用智能指针和 RAII 原则</li>
    <li><strong>编译优化</strong>：采用Clang最新版编译x64-V3版本，开启AVX2支持，性能更佳</li>
  </ul>

  <h2>🚀 技术亮点</h2>

  <h3>性能优化</h3>
  <ol>
    <li><strong>Qt6 + C++23 新特性</strong>
      <ul>
        <li>使用现代 C++23 特性提升代码质量和性能</li>
        <li>利用 Qt6 的最新优化和硬件加速功能</li>
        <li>采用范围 for 循环、结构化绑定、std::atomic原子智能指针等现代语法</li>
      </ul>
    </li>
    <li><strong>智能缓存系统</strong>
      <ul>
        <li>实现 LRU（Least Recently Used）缓存算法</li>
        <li>避免重复计算和不必要的资源加载</li>
        <li>智能内存管理，自动释放未使用的资源</li>
      </ul>
    </li>
    <li><strong>智能指针管理</strong>
      <ul>
        <li>全面使用 <code>std::shared_ptr</code> 和 <code>std::unique_ptr</code></li>
        <li>遵循 RAII（Resource Acquisition Is Initialization）原则</li>
        <li>自动内存管理，杜绝内存泄漏</li>
      </ul>
    </li>
    <li><strong>避免重复计算</strong>
      <ul>
        <li>缓存计算结果，避免重复执行相同操作</li>
        <li>智能状态管理，只在必要时更新界面</li>
        <li>优化几何计算和布局算法</li>
      </ul>
    </li>
  </ol>

  <h3>架构改进</h3>
  <ol>
    <li><strong>evix2 功能重写</strong>
      <ul>
        <li>完全重写了 evix2 功能模块</li>
        <li>避免了不必要的数据转换</li>
        <li>解决了 Windows 平台上的字符编码问题</li>
        <li>提供更稳定、更高效的文件处理能力</li>
      </ul>
    </li>
    <li><strong>严格编译检查</strong>
      <ul>
        <li>启用所有编译器警告</li>
        <li>使用静态分析工具检测潜在问题</li>
        <li>通过 GDB 调试确保代码质量</li>
      </ul>
    </li>
    <li><strong>模块化设计</strong>
      <ul>
        <li>清晰的组件分离和职责划分</li>
        <li>易于维护和扩展的代码结构</li>
        <li>良好的接口设计和依赖管理</li>
      </ul>
    </li>
  </ol>

  <h2>💥 破坏性更改</h2>

  <h3>缩略图系统移除</h3>
  <p><strong>原因</strong>：现代操作系统（如 Windows 资源管理器）已内置高质量缩略图功能</p>
  <p><strong>移除内容</strong>：</p>
  <ul>
    <li>缩略图请求系统</li>
    <li>缩略图计算逻辑</li>
    <li>缩略图生成和缓存机制</li>
    <li>相关的资源管理和内存开销</li>
  </ul>
  <p><strong>优势</strong>：</p>
  <ul>
    <li>显著减少内存使用</li>
    <li>提升应用启动速度</li>
    <li>避免重复的缩略图生成</li>
    <li>利用系统原生的缩略图缓存</li>
  </ul>

  <h2>📋 系统要求</h2>
  <ul>
    <li><strong>操作系统</strong>：Windows 10+</li>
    <li><strong>编译器</strong>：支持 C++23 的编译器（MSVC 2019+、GCC 7+、Clang 5+）</li>
    <li><strong>依赖库</strong>：Qt6（6.2+）、OpenCV等</li>
  </ul>

  <h2>🖥️ 使用方法</h2>

  <h3>基本操作</h3>
  <ul>
    <li><strong>打开图片</strong>：双击图片文件或拖拽到应用窗口</li>
    <li><strong>文件夹浏览</strong>：使用文件夹视图查看目录内容</li>
    <li><strong>图片查看</strong>：支持缩放、旋转、全屏等操作</li>
    <li><strong>键盘导航</strong>：使用方向键、Page Up/Down 等快捷键</li>
  </ul>

  <h3>快捷键</h3>
  <ul>
    <li><kbd>F11</kbd>：全屏切换</li>
    <li><kbd>Space</kbd>：播放/暂停动画</li>
    <li><kbd>Ctrl</kbd> + <kbd>+</kbd>/<kbd>-</kbd>：缩放图片</li>
    <li><kbd>Ctrl</kbd> + <kbd>O</kbd>：打开文件</li>
    <li><kbd>Ctrl</kbd> + <kbd>W</kbd>：关闭当前图片</li>
  </ul>

  <h3>代码规范</h3>
  <ul>
    <li>遵循 Google C++ 编码规范</li>
    <li>使用 clang-format 统一代码格式</li>
    <li>添加详细的代码注释和文档</li>
  </ul>

  <h3>贡献流程</h3>
  <ol>
    <li>Fork 项目仓库</li>
    <li>创建功能分支</li>
    <li>提交更改并编写测试</li>
    <li>提交 Pull Request</li>
  </ol>

  <hr />
  <p align="center">📧 个人修改项目，不接受功能请求，有bug可以提。</p>
<div id="en" class="lang-block">
  <h1 align="center">qimgv – Modern High-Performance Image Browsing Tool</h1>

  <p align="center">
    <strong>Completely refactored with Qt6 and C++23, delivering smooth, efficient, and intelligent image viewing.</strong>
  </p>

  <h2>📖 Project Overview</h2>
  <p>The codebase has been fully rewritten using Qt6 and C++23 standards, incorporating the latest programming techniques and best practices to provide a fluid and efficient image browsing experience.</p>

  <h2>✨ Key Features</h2>
  <ul>
    <li><strong>High-Performance Rendering</strong>: Leverages Qt6 hardware acceleration and optimized algorithms.</li>
    <li><strong>Modern Architecture</strong>: Modular design based on C++23 features.</li>
    <li><strong>Intelligent Memory Management</strong>: Utilizes smart pointers and RAII principles.</li>
    <li><strong>Compilation Optimization</strong>: Compiled with the latest Clang using x64-V3 architecture, adding AVX2 support for better performance.</li>
  </ul>

  <h2>🚀 Technical Highlights</h2>

  <h3>Performance Optimizations</h3>
  <ol>
    <li><strong>Qt6 + C++23 New Features</strong>
      <ul>
        <li>Modern C++23 features improve code quality and performance.</li>
        <li>Qt6's latest optimizations and hardware acceleration.</li>
        <li>Range-based for loops, structured bindings, std::atomic and other modern syntax.</li>
      </ul>
    </li>
    <li><strong>Intelligent Caching System</strong>
      <ul>
        <li>Implements LRU (Least Recently Used) caching algorithm.</li>
        <li>Avoids redundant calculations and unnecessary resource loading.</li>
        <li>Smart memory management automatically releases unused resources.</li>
      </ul>
    </li>
    <li><strong>Smart Pointer Management</strong>
      <ul>
        <li>Comprehensive use of <code>std::shared_ptr</code> and <code>std::unique_ptr</code>.</li>
        <li>Follows RAII (Resource Acquisition Is Initialization) principles.</li>
        <li>Automatic memory management eliminates memory leaks.</li>
      </ul>
    </li>
    <li><strong>Avoiding Redundant Computations</strong>
      <ul>
        <li>Caches results to prevent repeated operations.</li>
        <li>Smart state management updates UI only when necessary.</li>
        <li>Optimized geometry calculations and layout algorithms.</li>
      </ul>
    </li>
  </ol>

  <h3>Architectural Improvements</h3>
  <ol>
    <li><strong>evix2 Rewrite</strong>
      <ul>
        <li>Completely rewritten evix2 functionality module.</li>
        <li>Avoids unnecessary data conversions.</li>
        <li>Resolves character encoding issues on Windows.</li>
        <li>Provides more stable and efficient file handling.</li>
      </ul>
    </li>
    <li><strong>Strict Compilation Checks</strong>
      <ul>
        <li>Enables all compiler warnings.</li>
        <li>Uses static analysis tools to detect potential issues.</li>
        <li>Ensures code quality through GDB debugging.</li>
      </ul>
    </li>
    <li><strong>Modular Design</strong>
      <ul>
        <li>Clear separation of components and responsibilities.</li>
        <li>Easy to maintain and extend code structure.</li>
        <li>Well-defined interfaces and dependency management.</li>
      </ul>
    </li>
  </ol>

  <h2>💥 Breaking Changes</h2>

  <h3>Removal of Thumbnail System</h3>
  <p><strong>Reason</strong>: Modern operating systems (e.g., Windows Explorer) already provide high-quality built-in thumbnail functionality.</p>
  <p><strong>Removed Components</strong>:</p>
  <ul>
    <li>Thumbnail request system</li>
    <li>Thumbnail computation logic</li>
    <li>Thumbnail generation and caching mechanisms</li>
    <li>Associated resource management overhead</li>
  </ul>
  <p><strong>Advantages</strong>:</p>
  <ul>
    <li>Significant reduction in memory usage</li>
    <li>Faster application startup</li>
    <li>Eliminates redundant thumbnail generation</li>
    <li>Leverages OS-native thumbnail caching</li>
  </ul>

  <h2>📋 System Requirements</h2>
  <ul>
    <li><strong>OS</strong>: Windows 10+</li>
    <li><strong>Compiler</strong>: C++23 compatible (MSVC 2019+, GCC 7+, Clang 5+)</li>
    <li><strong>Libraries</strong>: Qt6 (6.2+), OpenCV, etc.</li>
  </ul>

  <h2>🖥️ Usage Guide</h2>

  <h3>Basic Operations</h3>
  <ul>
    <li><strong>Open Image</strong>: Double-click image file or drag & drop into the window.</li>
    <li><strong>Folder Browsing</strong>: Use folder view to navigate directory contents.</li>
    <li><strong>Image Viewing</strong>: Supports zoom, rotate, fullscreen, and more.</li>
    <li><strong>Keyboard Navigation</strong>: Arrow keys, Page Up/Down, and other shortcuts.</li>
  </ul>

  <h3>Keyboard Shortcuts</h3>
  <ul>
    <li><kbd>F11</kbd>: Toggle fullscreen</li>
    <li><kbd>Space</kbd>: Play/pause animation (if supported)</li>
    <li><kbd>Ctrl</kbd> + <kbd>+</kbd>/<kbd>-</kbd>: Zoom in/out</li>
    <li><kbd>Ctrl</kbd> + <kbd>O</kbd>: Open file</li>
    <li><kbd>Ctrl</kbd> + <kbd>W</kbd>: Close current image</li>
  </ul>

  <h3>Coding Standards</h3>
  <ul>
    <li>Follows Google C++ Style Guide.</li>
    <li>Code formatting enforced with clang-format.</li>
    <li>Detailed comments and documentation.</li>
  </ul>

  <h3>Contribution Workflow</h3>
  <ol>
    <li>Fork the repository.</li>
    <li>Create a feature branch.</li>
    <li>Commit changes with tests.</li>
    <li>Submit a Pull Request.</li>
  </ol>

  <hr />
  <p align="center">📧 For my personal use only. Feature requests are not accepted. You can report bugs though.</p
</div>
