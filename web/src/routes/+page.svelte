<script lang="ts">
	import { onMount } from 'svelte';
	import StatusBadge from '$lib/components/StatusBadge.svelte';
	import { system, monitors as monitorsApi } from '$lib/api';
	import { liveData } from '$lib/stores';
	import type { Monitor } from '$lib/api';

	let serverStatus = $state<any>(null);
	let monitorList = $state<Monitor[]>([]);

	// Derive live data
	let websites = $derived(($liveData as any).websites?.sites || []);
	let devices = $derived(($liveData as any).network?.devices || []);
	let cameras = $derived(($liveData as any).cameras?.cameras || []);

	let totalUp = $derived(websites.filter((s: any) => s.status === 'up').length);
	let totalDevOnline = $derived(devices.filter((d: any) => d.status === 'online').length);
	let avgMs = $derived(() => {
		const vals = websites.filter((s: any) => s.ms).map((s: any) => s.ms);
		return vals.length ? Math.round(vals.reduce((a: number, b: number) => a + b, 0) / vals.length) : 0;
	});

	onMount(async () => {
		try {
			serverStatus = await system.getStatus();
			monitorList = await monitorsApi.list();
		} catch {}
	});
</script>

<div class="space-y-6">
	<h2 class="text-xl font-semibold">Dashboard</h2>

	<!-- Summary cards -->
	<div class="grid grid-cols-2 lg:grid-cols-4 gap-4">
		<div class="p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
			<p class="text-xs text-[#8B949E] mb-1">Total Monitors</p>
			<p class="text-2xl font-bold text-[#E6EDF3]">{monitorList.length}</p>
		</div>
		<div class="p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
			<p class="text-xs text-[#8B949E] mb-1">Websites Up</p>
			<p class="text-2xl font-bold text-[#3FB950]">{totalUp}<span class="text-sm text-[#8B949E]">/{websites.length}</span></p>
		</div>
		<div class="p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
			<p class="text-xs text-[#8B949E] mb-1">Devices Online</p>
			<p class="text-2xl font-bold text-[#3FB950]">{totalDevOnline}<span class="text-sm text-[#8B949E]">/{devices.length}</span></p>
		</div>
		<div class="p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
			<p class="text-xs text-[#8B949E] mb-1">Avg Response</p>
			<p class="text-2xl font-bold text-[#58A6FF]">{avgMs()} <span class="text-sm">ms</span></p>
		</div>
	</div>

	<!-- Website status -->
	{#if websites.length > 0}
		<section>
			<h3 class="text-sm font-medium text-[#8B949E] mb-3">Website Status</h3>
			<div class="rounded-lg border border-[#30363D] overflow-hidden">
				{#each websites as site, i}
					<div class="flex items-center justify-between px-4 py-3 {i % 2 === 0 ? 'bg-[#161B22]' : 'bg-[#0D1117]'}">
						<div class="flex items-center gap-3">
							<StatusBadge status={site.status} />
							<span class="text-sm">{site.name}</span>
						</div>
						<span class="text-sm text-[#8B949E]">{site.ms ? `${site.ms} ms` : '--'}</span>
					</div>
				{/each}
			</div>
		</section>
	{/if}

	<!-- Network devices -->
	{#if devices.length > 0}
		<section>
			<h3 class="text-sm font-medium text-[#8B949E] mb-3">Network Devices</h3>
			<div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
				{#each devices as dev}
					<div class="p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
						<div class="flex items-center justify-between mb-2">
							<span class="text-sm font-medium">{dev.name}</span>
							<StatusBadge status={dev.status} />
						</div>
						<div class="flex items-center gap-4 text-xs text-[#8B949E]">
							<span>{dev.clients} clients</span>
							<span>{dev.bw_mbps} Mbps</span>
						</div>
					</div>
				{/each}
			</div>
		</section>
	{/if}

	<!-- Empty state -->
	{#if monitorList.length === 0 && !serverStatus}
		<div class="text-center py-12">
			<p class="text-[#8B949E] text-lg mb-2">No monitors configured yet</p>
			<p class="text-[#8B949E] text-sm mb-4">Add your first monitor to start tracking</p>
			<a href="/monitors" class="inline-block px-4 py-2 rounded-lg bg-[#58A6FF] text-white text-sm hover:bg-[#58A6FF]/80 transition">
				Add Monitor
			</a>
		</div>
	{/if}
</div>
