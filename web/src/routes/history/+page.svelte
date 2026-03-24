<script lang="ts">
	import { onMount } from 'svelte';
	import Chart from '$lib/components/Chart.svelte';
	import { monitors as monitorsApi, history as historyApi, type Monitor } from '$lib/api';
	import { notify } from '$lib/stores';

	let monitorList = $state<Monitor[]>([]);
	let selectedId = $state(0);
	let timeRange = $state('1h');
	let chartData = $state<number[]>([]);
	let chartLabels = $state<string[]>([]);
	let summary = $state<any>(null);

	const ranges: Record<string, number> = {
		'1h': 3600,
		'6h': 21600,
		'24h': 86400,
		'7d': 604800,
		'30d': 2592000,
	};

	onMount(async () => {
		try {
			monitorList = await monitorsApi.list();
			if (monitorList.length > 0) {
				selectedId = monitorList[0].id;
				await loadData();
			}
		} catch (e: any) {
			notify('error', e.message);
		}
	});

	async function loadData() {
		if (!selectedId) return;
		try {
			const now = Math.floor(Date.now() / 1000);
			const start = now - ranges[timeRange];
			const [dataRes, summaryRes] = await Promise.all([
				historyApi.getData(selectedId, start, now),
				historyApi.getSummary(selectedId, start, now),
			]);
			chartData = dataRes.data.map((d: any) => d.value).filter((v: any) => v != null);
			chartLabels = dataRes.data.map((d: any) => {
				const date = new Date(d.ts * 1000);
				return `${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;
			});
			summary = summaryRes;
		} catch (e: any) {
			notify('error', e.message);
		}
	}
</script>

<div class="space-y-6">
	<h2 class="text-xl font-semibold">History</h2>

	<!-- Controls -->
	<div class="flex flex-wrap items-center gap-4">
		<select
			bind:value={selectedId}
			onchange={loadData}
			class="px-3 py-2 rounded-lg bg-[#161B22] border border-[#30363D] text-sm text-[#E6EDF3]"
		>
			{#each monitorList as m}
				<option value={m.id}>{m.name} ({m.type})</option>
			{/each}
		</select>

		<div class="flex gap-1">
			{#each Object.keys(ranges) as range}
				<button
					onclick={() => { timeRange = range; loadData(); }}
					class="px-3 py-1.5 rounded text-xs transition
						{timeRange === range
							? 'bg-[#58A6FF] text-white'
							: 'bg-[#161B22] text-[#8B949E] border border-[#30363D] hover:text-[#E6EDF3]'}"
				>
					{range}
				</button>
			{/each}
		</div>
	</div>

	<!-- Chart -->
	{#if chartData.length > 0}
		<Chart data={chartData} labels={chartLabels} title="Response Time (ms)" />
	{:else}
		<div class="h-64 rounded-lg bg-[#161B22] border border-[#30363D] flex items-center justify-center text-[#8B949E]">
			{monitorList.length === 0 ? 'No monitors configured' : 'No data for selected range'}
		</div>
	{/if}

	<!-- Summary stats -->
	{#if summary}
		<div class="grid grid-cols-2 md:grid-cols-5 gap-4">
			<div class="p-3 rounded-lg bg-[#161B22] border border-[#30363D]">
				<p class="text-xs text-[#8B949E]">Avg</p>
				<p class="text-lg font-bold">{summary.avg_value} ms</p>
			</div>
			<div class="p-3 rounded-lg bg-[#161B22] border border-[#30363D]">
				<p class="text-xs text-[#8B949E]">Min</p>
				<p class="text-lg font-bold">{summary.min_value ?? '--'} ms</p>
			</div>
			<div class="p-3 rounded-lg bg-[#161B22] border border-[#30363D]">
				<p class="text-xs text-[#8B949E]">Max</p>
				<p class="text-lg font-bold">{summary.max_value ?? '--'} ms</p>
			</div>
			<div class="p-3 rounded-lg bg-[#161B22] border border-[#30363D]">
				<p class="text-xs text-[#8B949E]">Uptime</p>
				<p class="text-lg font-bold text-[#3FB950]">{summary.uptime_pct}%</p>
			</div>
			<div class="p-3 rounded-lg bg-[#161B22] border border-[#30363D]">
				<p class="text-xs text-[#8B949E]">Checks</p>
				<p class="text-lg font-bold">{summary.total_checks}</p>
			</div>
		</div>
	{/if}
</div>
