<script lang="ts">
	import { onMount } from 'svelte';
	import { display as displayApi, type PageConfig } from '$lib/api';
	import { notify } from '$lib/stores';

	let pages = $state<PageConfig[]>([]);

	onMount(async () => {
		try {
			pages = await displayApi.getPages();
		} catch (e: any) {
			notify('error', e.message);
		}
	});

	function moveUp(index: number) {
		if (index <= 0) return;
		const temp = pages[index];
		pages[index] = pages[index - 1];
		pages[index - 1] = temp;
		pages = pages.map((p, i) => ({ ...p, page_index: i }));
	}

	function moveDown(index: number) {
		if (index >= pages.length - 1) return;
		const temp = pages[index];
		pages[index] = pages[index + 1];
		pages[index + 1] = temp;
		pages = pages.map((p, i) => ({ ...p, page_index: i }));
	}

	async function saveLayout() {
		try {
			await displayApi.updatePages(pages);
			notify('success', 'Display layout updated and pushed to ESP');
		} catch (e: any) {
			notify('error', e.message);
		}
	}
</script>

<div class="space-y-6">
	<div class="flex items-center justify-between">
		<h2 class="text-xl font-semibold">Display Layout</h2>
		<button onclick={saveLayout} class="px-4 py-2 rounded-lg bg-[#3FB950] text-white text-sm hover:bg-[#3FB950]/80 transition">
			Push to ESP
		</button>
	</div>

	<div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
		<!-- Page list -->
		<div class="space-y-3">
			<h3 class="text-sm font-medium text-[#8B949E]">Pages (drag to reorder)</h3>
			{#each pages as page, i (page.page_index)}
				<div class="flex items-center gap-3 p-4 rounded-lg bg-[#161B22] border border-[#30363D]">
					<div class="flex flex-col gap-1">
						<button onclick={() => moveUp(i)} class="text-xs text-[#8B949E] hover:text-[#E6EDF3]" disabled={i === 0}>&#9650;</button>
						<button onclick={() => moveDown(i)} class="text-xs text-[#8B949E] hover:text-[#E6EDF3]" disabled={i === pages.length - 1}>&#9660;</button>
					</div>
					<span class="text-sm text-[#58A6FF] font-mono w-6">{i}</span>
					<div class="flex-1">
						<input bind:value={page.title} class="w-full px-2 py-1 rounded bg-[#0D1117] border border-[#30363D] text-sm text-[#E6EDF3]" />
					</div>
					<span class="text-xs text-[#8B949E] bg-[#1C2128] px-2 py-1 rounded">{page.page_type}</span>
				</div>
			{/each}
		</div>

		<!-- ESP Preview -->
		<div>
			<h3 class="text-sm font-medium text-[#8B949E] mb-3">ESP Preview (240x320)</h3>
			<div class="w-60 h-80 bg-[#0D1117] rounded-lg border-2 border-[#30363D] mx-auto overflow-hidden relative">
				<!-- Top bar -->
				<div class="h-4 bg-[#161B22] flex items-center justify-between px-2">
					<span class="text-[8px] text-[#E6EDF3]">MonDash</span>
					<span class="text-[8px] text-[#8B949E]">12:34</span>
				</div>
				<!-- Content area -->
				<div class="flex-1 p-2">
					<div class="text-[9px] text-[#8B949E] text-center mt-16">
						{pages[0]?.title || 'Overview'}
					</div>
					<div class="grid grid-cols-2 gap-1 mt-2">
						{#each Array(6) as _, i}
							<div class="h-6 bg-[#161B22] rounded text-[7px] text-[#8B949E] flex items-center justify-center">
								Card {i + 1}
							</div>
						{/each}
					</div>
				</div>
				<!-- Page dots -->
				<div class="absolute bottom-2 left-0 right-0 flex justify-center gap-1">
					{#each pages as _, i}
						<div class="w-1.5 h-1.5 rounded-full {i === 0 ? 'bg-[#58A6FF]' : 'bg-[#8B949E]/50'}"></div>
					{/each}
				</div>
			</div>
		</div>
	</div>
</div>
