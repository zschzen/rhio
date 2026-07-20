import { db } from './data.js';
import { resolveExampleAsset, getExampleLoader, getExampleWasm } from './util.js';

function getRuntimeParts(runtime) {
  return {
    canvas: runtime.querySelector('[data-runtime-canvas]'),
    status: runtime.querySelector('[data-runtime-status]'),
    progress: runtime.querySelector('[data-runtime-progress]'),
    output: runtime.querySelector('[data-runtime-output]'),
    outputPane: runtime.querySelector('[data-runtime-output-pane]'),
    outputCount: runtime.querySelector('[data-runtime-output-count]'),
    fullscreen: runtime.querySelector('[data-runtime-fullscreen]'),
    reload: runtime.querySelector('[data-runtime-reload]')
  };
}

function setRuntimeStatus(runtime, message, options = {}) {
  const { status, progress } = getRuntimeParts(runtime);
  const text = message || 'Running';

  runtime.dataset.state = options.state || runtime.dataset.state || 'loading';
  if (status) status.textContent = text;

  if (!progress) return;

  const match = text.match(/([^(]+)\((\d+(?:\.\d+)?)\/(\d+)\)/);
  if (match) {
    progress.hidden = false;
    progress.value = Number(match[2]);
    progress.max = Number(match[3]);
  } else if (options.hideProgress !== false) {
    progress.hidden = true;
    progress.removeAttribute('value');
  }
}

function appendRuntimeOutput(runtime, text) {
  const { output, outputCount } = getRuntimeParts(runtime);
  if (!output) return;

  output.value += `${text}\n`;
  output.scrollTop = output.scrollHeight;

  if (outputCount) {
    const lines = output.value.split('\n').length - 1;
    outputCount.textContent = String(lines);
  }
}

function resetRuntimeOutput(runtime) {
  const { output, outputPane, outputCount } = getRuntimeParts(runtime);
  if (output) output.value = '';
  if (outputCount) outputCount.textContent = '0';
  if (outputPane) outputPane.open = false;
}

function recreateCanvas(runtime) {
  const oldCanvas = runtime.querySelector('[data-runtime-canvas]');
  if (!oldCanvas) return;
  const clone = oldCanvas.cloneNode(false);
  oldCanvas.replaceWith(clone);
}

function createRuntimeModule(runtime, ex) {
  const { canvas, fullscreen } = getRuntimeParts(runtime);
  const wasmFile = getExampleWasm(ex);

  if (!canvas) throw new Error('Example runtime canvas was not found.');
  if (!wasmFile) throw new Error('Example WebAssembly binary was not configured.');

  canvas.addEventListener('webglcontextlost', event => {
    event.preventDefault();
    if (runtime.dataset.state !== 'idle') {
      setRuntimeStatus(runtime, 'WebGL context lost', { state: 'error' });
    }
  });

  if (fullscreen && !fullscreen.dataset.bound) {
    fullscreen.dataset.bound = '1';
    fullscreen.addEventListener('click', async () => {
      const target = runtime.querySelector('.runtime-canvas-wrap') || canvas;
      if (document.fullscreenElement) {
        await document.exitFullscreen();
      } else if (target.requestFullscreen) {
        await target.requestFullscreen();
      }
    });
  }

  return {
    canvas,
    locateFile(path) {
      return new URL(resolveExampleAsset(ex, path), document.baseURI).href;
    },
    print(...args) {
      appendRuntimeOutput(runtime, args.join(' '));
    },
    printErr(...args) {
      appendRuntimeOutput(runtime, args.join(' '));
    },
    setStatus(message) {
      setRuntimeStatus(runtime, message, { state: message ? 'loading' : 'running' });
    },
    monitorRunDependencies(left) {
      this.totalDependencies = Math.max(this.totalDependencies || 0, left);
      setRuntimeStatus(
        runtime,
        left ? `Preparing... (${this.totalDependencies - left}/${this.totalDependencies})` : 'All downloads complete.',
        { state: left ? 'loading' : 'running', hideProgress: false }
      );
    },
    onRuntimeInitialized() {
      setRuntimeStatus(runtime, 'Running', { state: 'running' });
    },
    onAbort(reason) {
      setRuntimeStatus(runtime, `Aborted: ${reason}`, { state: 'error' });
    }
  };
}

export async function initExampleRuntime(exId) {
  const ex = db[exId];
  const runtime = document.querySelector('[data-example-runtime]');

  if (!ex || !runtime) return;
  if (runtime.dataset.state === 'loading' || runtime.dataset.state === 'running') return;

  const loader = getExampleLoader(ex);
  if (!loader) {
    setRuntimeStatus(runtime, 'JavaScript loader missing', { state: 'error' });
    return;
  }

  const loadToken = `${exId}/${Date.now()}`;
  const loaderUrl = resolveExampleAsset(ex, loader);
  runtime.dataset.loadToken = loadToken;
  setRuntimeStatus(runtime, 'Downloading...', { state: 'loading' });

  try {
    const moduleConfig = createRuntimeModule(runtime, ex);
    const response = await fetch(loaderUrl, { credentials: 'same-origin' });

    if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);

    const source = await response.text();
    if (!runtime.isConnected || runtime.dataset.loadToken !== loadToken) return;

    const sourceUrl = new URL(loaderUrl, document.baseURI).href;
    const execute = new Function('Module', `${source}\n//# sourceURL=${sourceUrl}`);
    execute(moduleConfig);
  } catch (error) {
    if (runtime.isConnected && runtime.dataset.loadToken === loadToken) {
      setRuntimeStatus(runtime, 'Failed to load example', { state: 'error' });
      appendRuntimeOutput(runtime, error instanceof Error ? error.message : String(error));
    }
    console.error(error);
  }
}

export function setupRuntimeReload(exId) {
  const runtime = document.querySelector('[data-example-runtime]');
  if (!runtime) return;
  const { reload, canvas } = getRuntimeParts(runtime);
  if (!reload || reload.dataset.bound === '1') return;

  reload.dataset.bound = '1';
  reload.addEventListener('click', () => {
    recreateCanvas(runtime);
    resetRuntimeOutput(runtime);
    runtime.dataset.state = 'idle';
    delete runtime.dataset.loadToken;
    setRuntimeStatus(runtime, 'Idle', { state: 'idle' });
    initExampleRuntime(exId);
  });
}

export async function loadExampleSource(exId) {
  const ex = db[exId];
  const codeEl = document.querySelector('[data-example-source]');
  if (!ex?.source || !codeEl) return;
  try {
    const res = await fetch(ex.source, { credentials: 'same-origin' });
    if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
    codeEl.textContent = await res.text();
    window.Prism?.highlightElement(codeEl);
  } catch (error) {
    codeEl.textContent = `Failed to load source: ${error.message}`;
  }
}
