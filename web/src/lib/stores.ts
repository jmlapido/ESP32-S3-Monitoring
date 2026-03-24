import { writable } from 'svelte/store';
import type { Monitor } from './api';

export const monitorsStore = writable<Monitor[]>([]);
export const espConnected = writable(false);
export const notifications = writable<{ id: number; type: string; message: string }[]>([]);

let notifId = 0;
export function notify(type: 'success' | 'error' | 'warning' | 'info', message: string) {
	const id = ++notifId;
	notifications.update((n) => [...n, { id, type, message }]);
	setTimeout(() => {
		notifications.update((n) => n.filter((x) => x.id !== id));
	}, 4000);
}

// WebSocket connection to /ws/web
let ws: WebSocket | null = null;
let reconnectTimer: ReturnType<typeof setTimeout> | null = null;
export const liveData = writable<Record<string, any>>({});

export function connectWebSocket() {
	if (ws && ws.readyState === WebSocket.OPEN) return;

	const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
	ws = new WebSocket(`${proto}//${location.host}/ws/web`);

	ws.onopen = () => {
		espConnected.set(true);
		if (reconnectTimer) clearTimeout(reconnectTimer);
	};

	ws.onmessage = (event) => {
		try {
			const msg = JSON.parse(event.data);
			if (msg.type === 'update' || msg.type === 'chart_data') {
				liveData.update((d) => ({ ...d, [msg.page]: msg.data }));
			}
		} catch {}
	};

	ws.onclose = () => {
		espConnected.set(false);
		reconnectTimer = setTimeout(connectWebSocket, 3000);
	};

	ws.onerror = () => {
		ws?.close();
	};
}
