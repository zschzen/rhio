export function escapeHtml(unsafe) {
  return String(unsafe)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

export function resolveExampleAsset(ex, file) {
  return `${ex.assetBase.replace(/\/$/, '')}/${file}`;
}

export function getExampleLoader(ex) {
  return ex.loader || (Array.isArray(ex.files) ? ex.files.find(file => file.endsWith('.js')) : null);
}

export function getExampleWasm(ex) {
  return ex.wasm || (Array.isArray(ex.files) ? ex.files.find(file => file.endsWith('.wasm')) : null);
}
