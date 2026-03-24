const BASE = '';

async function request<T>(path: string, options?: RequestInit): Promise<T> {
	const res = await fetch(`${BASE}${path}`, {
		headers: { 'Content-Type': 'application/json' },
		...options,
	});
	if (!res.ok) {
		const err = await res.json().catch(() => ({ detail: res.statusText }));
		throw new Error(err.detail || `HTTP ${res.status}`);
	}
	return res.json();
}

// Monitors
export interface Monitor {
	id: number;
	type: string;
	name: string;
	target: string;
	config: Record<string, any>;
	poll_interval_sec: number;
	enabled: boolean;
	page_assignment: string;
	display_order: number;
	created_at: number;
	updated_at: number;
}

export interface MonitorCreate {
	type: string;
	name: string;
	target: string;
	config?: Record<string, any>;
	poll_interval_sec?: number;
	enabled?: boolean;
}

export const monitors = {
	list: () => request<Monitor[]>('/api/monitors/'),
	get: (id: number) => request<Monitor>(`/api/monitors/${id}`),
	create: (data: MonitorCreate) => request<Monitor>('/api/monitors/', { method: 'POST', body: JSON.stringify(data) }),
	update: (id: number, data: Partial<MonitorCreate>) => request<Monitor>(`/api/monitors/${id}`, { method: 'PUT', body: JSON.stringify(data) }),
	delete: (id: number) => request<{ deleted: boolean }>(`/api/monitors/${id}`, { method: 'DELETE' }),
	test: (id: number) => request<any>(`/api/monitors/${id}/test`, { method: 'POST' }),
};

// Display
export interface PageConfig {
	id?: number;
	page_index: number;
	page_type: string;
	title: string;
	config: Record<string, any>;
}

export const display = {
	getPages: () => request<PageConfig[]>('/api/display/pages'),
	updatePages: (pages: PageConfig[]) => request<any>('/api/display/pages', { method: 'PUT', body: JSON.stringify(pages) }),
	preview: () => request<any[]>('/api/display/preview'),
};

// History
export const history = {
	getData: (monitorId: number, start?: number, end?: number, resolution?: string) => {
		const params = new URLSearchParams();
		if (start) params.set('start', String(start));
		if (end) params.set('end', String(end));
		if (resolution) params.set('resolution', resolution);
		return request<any>(`/api/history/${monitorId}/data?${params}`);
	},
	getSummary: (monitorId: number, start?: number, end?: number) => {
		const params = new URLSearchParams();
		if (start) params.set('start', String(start));
		if (end) params.set('end', String(end));
		return request<any>(`/api/history/${monitorId}/summary?${params}`);
	},
};

// System
export const system = {
	getStatus: () => request<any>('/api/system/status'),
	getEspStatus: () => request<any>('/api/system/esp/status'),
	getConfig: () => request<any>('/api/system/config'),
};
