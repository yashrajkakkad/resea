
export async function fetchJson(path, options) {
    return await (await fetch(path, options)).json();
}
