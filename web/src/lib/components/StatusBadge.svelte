<script lang="ts">
	let { status = 'unknown' }: { status: string } = $props();

	const colors: Record<string, string> = {
		up: 'bg-[#3FB950]',
		online: 'bg-[#3FB950]',
		ok: 'bg-[#3FB950]',
		down: 'bg-[#F85149]',
		offline: 'bg-[#F85149]',
		warning: 'bg-[#D29922]',
		unknown: 'bg-[#8B949E]',
	};

	const labels: Record<string, string> = {
		up: 'Online',
		online: 'Online',
		ok: 'OK',
		down: 'Offline',
		offline: 'Offline',
		warning: 'Warning',
		unknown: 'Unknown',
	};

	let dotClass = $derived(colors[status] || colors.unknown);
	let isPulsing = $derived(status === 'down' || status === 'offline');
</script>

<span class="inline-flex items-center gap-1.5 text-xs">
	<span class="w-2 h-2 rounded-full {dotClass} {isPulsing ? 'pulse-offline' : ''}"></span>
	<span class="text-[#8B949E]">{labels[status] || status}</span>
</span>
