import { db } from '../data.js';
import { escapeHtml } from '../util.js';

export default function exampleDetail(exId) {
  const ex = db[exId];
  if (!ex) {
    return `<div class="container"><h2>Example not found</h2><p><a href="#/examples">Back to examples</a></p></div>`;
  }

  const tagsHtml = ex.tags ? ex.tags.map(t => `<span class="tag">${escapeHtml(t)}</span>`).join('') : '';
  const tagsBlock = tagsHtml ? `<div class="meta-tags">${tagsHtml}</div>` : '';
  const ledeBlock = ex.desc ? `<p class="example-lede">${escapeHtml(ex.desc)}</p>` : '';
  const sourceBlock = ex.source ? `
    <details class="source-pane">
      <summary>Show source <code>${escapeHtml(ex.sourceName || ex.source)}</code></summary>
      <pre class="line-numbers"><code class="language-cpp" data-example-source>Loading...</code></pre>
    </details>
  ` : '';

  return `
    <div class="container">
      <nav aria-label="Breadcrumb">
        <ul class="breadcrumb">
          <li class="breadcrumb-item"><a href="#/examples">Examples</a></li>
          <li class="breadcrumb-item current" aria-current="page">${escapeHtml(ex.title)}</li>
        </ul>
      </nav>

      <header class="example-header">
        <h2 class="example-title">${escapeHtml(ex.title)}</h2>
        ${ledeBlock}
        ${tagsBlock}
      </header>

      <div class="example-runtime" data-example-runtime data-state="idle">
        <div class="runtime-canvas-wrap">
          <canvas data-runtime-canvas tabindex="-1" oncontextmenu="event.preventDefault()"></canvas>
        </div>
        <div class="runtime-hud">
          <span class="status-dot" data-runtime-dot aria-hidden="true"></span>
          <span data-runtime-status>Idle</span>
          <progress data-runtime-progress hidden></progress>
          <div class="hud-actions">
            <button data-runtime-reload type="button" class="btn-hud">
              <svg viewBox="0 0 32 32" aria-hidden="true"><path fill="currentColor" d="M12 10H6.78A11 11 0 0 1 27 16h2A13 13 0 0 0 6 7.68V4H4v8h8zM20 22h5.22A11 11 0 0 1 5 16H3a13 13 0 0 0 23 8.32V28h2v-8h-8z"/></svg>
              <span>Reload</span>
            </button>
            <button data-runtime-fullscreen type="button" class="btn-hud">
              <svg viewBox="0 0 32 32" aria-hidden="true"><path fill="currentColor" d="M4 18v10h10v-2H6V18Zm14-14v2h8v8h2V4z"/></svg>
              <span>Fullscreen</span>
            </button>
          </div>
        </div>
        <details class="output-pane" data-runtime-output-pane>
          <summary>Output <span class="pane-count" data-runtime-output-count>0</span></summary>
          <textarea data-runtime-output readonly></textarea>
        </details>
      </div>

      ${sourceBlock}
    </div>
  `;
}
