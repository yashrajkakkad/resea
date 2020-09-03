
export async function fetchJson(path, options) {
    const resp = await fetch(path, options);
    if (resp.status !== 200) {
        const body = await resp.text();
        throw new Error(`unexpected response: ${resp.status}: ${body}`);
    }
    return await resp.json();
}
