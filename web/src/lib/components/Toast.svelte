<script lang="ts">
	import { notifications } from '$lib/stores';

	const typeStyles: Record<string, string> = {
		success: 'border-[#3FB950] bg-[#3FB950]/10',
		error: 'border-[#F85149] bg-[#F85149]/10',
		warning: 'border-[#D29922] bg-[#D29922]/10',
		info: 'border-[#58A6FF] bg-[#58A6FF]/10',
	};
</script>

<div class="fixed top-4 right-4 z-50 flex flex-col gap-2">
	{#each $notifications as notif (notif.id)}
		<div
			class="px-4 py-3 rounded-lg border text-sm text-[#E6EDF3] shadow-lg backdrop-blur-sm {typeStyles[notif.type] || typeStyles.info}"
			role="alert"
		>
			{notif.message}
		</div>
	{/each}
</div>
