<script lang="ts">
	import { onMount } from 'svelte';
	import { system } from '$lib/api';
	import { notify } from '$lib/stores';

	let serverStatus = $state<any>(null);
	let espStatus = $state<any>(null);
	let config = $state<any>(null);

	onMount(async () => {
		try {
			[serverStatus, espStatus, config] = await Promise.all([
				system.getStatus(),
				system.getEspStatus(),
				system.getConfig(),
			]);
		} catch (e: any) {
			notify('error', e.message);
		}
	});

	function formatUptime(seconds: number): string {
		const d = Math.floor(seconds / 86400);
		const h = Math.floor((seconds % 86400) / 3600);
		const m = Math.floor((seconds % 3600) / 60);
		return `${d}d ${h}h ${m}m`;
	}

	function formatBytes(bytes: number): string {
		if (bytes < 1024) return `${bytes} B`;
		if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`;
		return `${(bytes / 1048576).toFixed(1)} MB`;
	}
</script>

<div class="space-y-6">
	<h2 class="text-xl font-semibold">Settings</h2>

	<!-- Server Info -->
	<section class="p-6 rounded-lg bg-[#161B22] border border-[#30363D]">
		<h3 class="text-sm font-medium text-[#8B949E] mb-4">Server Info</h3>
		{#if serverStatus}
			<div class="grid grid-cols-2 md:grid-cols-4 gap-4">
				<div>
					<p class="text-xs text-[#8B949E]">Version</p>
					<p class="text-sm">{serverStatus.version}</p>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Uptime</p>
					<p class="text-sm">{formatUptime(serverStatus.uptime_seconds)}</p>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Database Size</p>
					<p class="text-sm">{formatBytes(serverStatus.database_size_bytes)}</p>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Web Clients</p>
					<p class="text-sm">{serverStatus.web_clients}</p>
				</div>
			</div>
		{:else}
			<p class="text-sm text-[#8B949E]">Loading...</p>
		{/if}
	</section>

	<!-- ESP Connection -->
	<section class="p-6 rounded-lg bg-[#161B22] border border-[#30363D]">
		<h3 class="text-sm font-medium text-[#8B949E] mb-4">ESP32 Connection</h3>
		{#if espStatus}
			<div class="grid grid-cols-2 md:grid-cols-3 gap-4">
				<div>
					<p class="text-xs text-[#8B949E]">Status</p>
					<div class="flex items-center gap-2">
						<span class="w-2 h-2 rounded-full {espStatus.connected ? 'bg-[#3FB950]' : 'bg-[#F85149]'}"></span>
						<p class="text-sm">{espStatus.connected ? 'Connected' : 'Disconnected'}</p>
					</div>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Last Heartbeat</p>
					<p class="text-sm">{espStatus.last_heartbeat ? new Date(espStatus.last_heartbeat * 1000).toLocaleTimeString() : '--'}</p>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Device</p>
					<p class="text-sm">{espStatus.device_info?.device || 'Unknown'}</p>
				</div>
			</div>
		{:else}
			<p class="text-sm text-[#8B949E]">Loading...</p>
		{/if}
	</section>

	<!-- Data Retention -->
	<section class="p-6 rounded-lg bg-[#161B22] border border-[#30363D]">
		<h3 class="text-sm font-medium text-[#8B949E] mb-4">Data Retention</h3>
		{#if config}
			<div class="grid grid-cols-2 gap-4">
				<div>
					<p class="text-xs text-[#8B949E]">Detailed Data</p>
					<p class="text-sm">{config.data_retention_days} days</p>
				</div>
				<div>
					<p class="text-xs text-[#8B949E]">Hourly Aggregates</p>
					<p class="text-sm">{config.hourly_retention_days} days</p>
				</div>
			</div>
		{/if}
	</section>
</div>
