<script lang="ts">
	import StatusBadge from './StatusBadge.svelte';
	import type { Monitor } from '$lib/api';

	let { monitor, onclick }: { monitor: Monitor; onclick?: () => void } = $props();

	const typeIcons: Record<string, string> = {
		http: '🌐',
		snmp: '📡',
		rtsp: '📷',
	};
</script>

<button
	class="w-full text-left p-4 rounded-lg border border-[#30363D] bg-[#161B22] hover:bg-[#1C2128] hover:border-[#58A6FF]/30 transition-all cursor-pointer"
	onclick={onclick}
>
	<div class="flex items-start justify-between">
		<div class="flex items-center gap-2">
			<span class="text-lg">{typeIcons[monitor.type] || '📊'}</span>
			<div>
				<h3 class="text-sm font-medium text-[#E6EDF3]">{monitor.name}</h3>
				<p class="text-xs text-[#8B949E] truncate max-w-[200px]">{monitor.target}</p>
			</div>
		</div>
		<StatusBadge status={monitor.enabled ? 'unknown' : 'offline'} />
	</div>
</button>
