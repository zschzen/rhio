export default function home() {
  return `
    <div class="hero">
      <div>
        <h1>rhio.h</h1>
        <p>A single-file, header-only Render Hardware Interface written in C99. Explicit vtables, manual resource management, and out-of-the-box backends for OpenGL and Vulkan.</p>
        <div class="btn-group">
          <a href="#/examples" class="btn btn-secondary">
            <span>View examples</span>
            <svg viewBox="0 0 32 32"><path d="M26,22v4H6V22H4v4c0,1.1,0.9,2,2,2h20c1.1,0,2-0.9,2-2v-4H26z M17,16l4.6-4.6l-1.4-1.4l-3.2,3.2V4h-2v9.2l-3.2-3.2l-1.4,1.4L15,16V16z"></path></svg>
          </a>
          <a href="https://github.com/zschzen/rhio" class="btn btn-primary" target="_blank" rel="noopener noreferrer">
            <span>View on GitHub</span>
            <svg viewBox="0 0 32 32"><path d="M16.69 4.7l-1.42 1.41L26.17 15H4v2h22.17l-10.9 10.9 1.42 1.4L28.81 16 16.69 4.7z"></path></svg>
          </a>
        </div>
      </div>
    </div>
  `;
}
