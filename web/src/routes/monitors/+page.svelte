<script lang="ts">
	import { onMount } from 'svelte';
	import { monitors as monitorsApi, type Monitor, type MonitorCreate } from '$lib/api';
	import MonitorCard from '$lib/components/MonitorCard.svelte';
	import StatusBadge from '$lib/components/StatusBadge.svelte';
	import { notify } from '$lib/stores';

	let monitorList = $state<Monitor[]>([]);
	let showForm = $state(false);
	let search = $state('');
	let form = $state<MonitorCreate>({ type: 'http', name: '', target: '', poll_interval_sec: 60 });

	let filtered = $derived(
		monitorList.filter((m) =>
			m.name.toLowerCase().includes(search.toLowerCase()) ||
			m.target.toLowerCase().includes(search.toLowerCase())
		)
	);

	onMount(async () => {
		await loadMonitors();
	});

	async function loadMonitors() {
		try {
			monitorList = await monitorsApi.list();
		} catch (e: any) {
			notify('error', e.message);
		}
	}

	async function createMonitor() {
		try {
			await monitorsApi.create(form);
			notify('success', `Monitor "${form.name}" created`);
			showForm = false;
			form = { type: 'http', name: '', target: '', poll_interval_sec: 60 };
			await loadMonitors();
		} catch (e: any) {
			notify('error', e.message);
		}
	}

	async function deleteMonitor(id: number, name: string) {
		try {
			await monitorsApi.delete(id);
			notify('success', `Monitor "${name}" deleted`);
			await loadMonitors();
		} catch (e: any) {
			notify('error', e.message);
		}
	}

	async function testMonitor(id: number) {
		try {
			const result = await monitorsApi.test(id);
			notify('info', `Test result: ${result.status} ${result.value_text || ''}`);
		} catch (e: any) {
			notify('error', e.message);
		}
	}
</script>

<div class="space-y-6">
	<div class="flex items-center justify-between">
		<h2 class="text-xl font-semibold">Monitors</h2>
		<button
			onclick={() => (showForm = !showForm)}
			class="px-4 py-2 rounded-lg bg-[#58A6FF] text-white text-sm hover:bg-[#58A6FF]/80 transition"
		>
			{showForm ? 'Cancel' : '+ Add Monitor'}
		</button>
	</div>

	<!-- Add monitor form -->
	{#if showForm}
		<div class="p-6 rounded-lg bg-[#161B22] border border-[#30363D] space-y-4">
			<h3 class="text-sm font-medium text-[#8B949E]">New Monitor</h3>

			<div class="grid grid-cols-1 md:grid-cols-2 gap-4">
				<div>
					<label class="block text-xs text-[#8B949E] mb-1">Type</label>
					<select bind:value={form.type} class="w-full px-3 py-2 rounded-lg bg-[#0D1117] border border-[#30363D] text-sm text-[#E6EDF3]">
						<option value="http">HTTP Website</option>
						<option value="snmp">SNMP Network Device</option>
						<option value="rtsp">RTSP Camera</option>
					</select>
				</div>
				<div>
					<label class="block text-xs text-[#8B949E] mb-1">Name</label>
					<input bind:value={form.name} placeholder="My Server" class="w-full px-3 py-2 rounded-lg bg-[#0D1117] border border-[#30363D] text-sm text-[#E6EDF3] placeholder-[#8B949E]" />
				</div>
				<div class="md:col-span-2">
					<label class="block text-xs text-[#8B949E] mb-1">
						{form.type === 'http' ? 'URL' : form.type === 'snmp' ? 'IP Address' : 'RTSP URI'}
					</label>
					<input
						bind:value={form.target}
						placeholder={form.type === 'http' ? 'https://example.com' : form.type === 'snmp' ? '192.168.1.1' : 'rtsp://192.168.1.100:554/stream'}
						class="w-full px-3 py-2 rounded-lg bg-[#0D1117] border border-[#30363D] text-sm text-[#E6EDF3] placeholder-[#8B949E]"
					/>
				</div>
				<div>
					<label class="block text-xs text-[#8B949E] mb-1">Check Interval (seconds)</label>
					<input type="number" bind:value={form.poll_interval_sec} min="5" class="w-full px-3 py-2 rounded-lg bg-[#0D1117] border border-[#30363D] text-sm text-[#E6EDF3]" />
				</div>
			</div>

			<button onclick={createMonitor} class="px-4 py-2 rounded-lg bg-[#3FB950] text-white text-sm hover:bg-[#3FB950]/80 transition">
				Create Monitor
			</button>
		</div>
	{/if}

	<!-- Search -->
	<input
		bind:value={search}
		placeholder="Search monitors..."
		class="w-full px-4 py-2 rounded-lg bg-[#161B22] border border-[#30363D] text-sm text-[#E6EDF3] placeholder-[#8B949E]"
	/>

	<!-- Monitor list -->
	{#if filtered.length > 0}
		<div class="space-y-3">
			{#each filtered as monitor (monitor.id)}
				<div class="flex items-center gap-3 p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
					<span class="text-lg">{monitor.type === 'http' ? '🌐' : monitor.type === 'snmp' ? '📡' : '📷'}</span>
					<div class="flex-1 min-w-0">
						<h3 class="text-sm font-medium truncate">{monitor.name}</h3>
						<p class="text-xs text-[#8B949E] truncate">{monitor.target}</p>
					</div>
					<span class="text-xs text-[#8B949E]">{monitor.poll_interval_sec}s</span>
					<StatusBadge status={monitor.enabled ? 'unknown' : 'offline'} />
					<div class="flex gap-2">
						<button onclick={() => testMonitor(monitor.id)} class="px-2 py-1 rounded text-xs bg-[#1C2128] text-[#58A6FF] hover:bg-[#58A6FF]/10 border border-[#30363D]">
							Test
						</button>
						<button onclick={() => deleteMonitor(monitor.id, monitor.name)} class="px-2 py-1 rounded text-xs bg-[#1C2128] text-[#F85149] hover:bg-[#F85149]/10 border border-[#30363D]">
							Delete
						</button>
					</div>
				</div>
			{/each}
		</div>
	{:else}
		<div class="text-center py-12 text-[#8B949E]">
			{search ? 'No monitors match your search' : 'No monitors yet. Click "Add Monitor" to get started.'}
		</div>
	{/if}
</div>
