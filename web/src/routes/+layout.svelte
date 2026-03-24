<script lang="ts">
	import '../app.css';
	import Toast from '$lib/components/Toast.svelte';
	import { espConnected, connectWebSocket } from '$lib/stores';
	import { onMount } from 'svelte';
	import { page } from '$app/stores';

	let { children } = $props();
	let sidebarOpen = $state(true);

	const navItems = [
		{ href: '/', label: 'Dashboard', icon: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6' },
		{ href: '/monitors', label: 'Monitors', icon: 'M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z' },
		{ href: '/display', label: 'Display', icon: 'M9.75 17L9 20l-1 1h8l-1-1-.75-3M3 13h18M5 17h14a2 2 0 002-2V5a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z' },
		{ href: '/history', label: 'History', icon: 'M7 12l3-3 3 3 4-4M8 21l4-4 4 4M3 4h18M4 4h16v12a1 1 0 01-1 1H5a1 1 0 01-1-1V4z' },
		{ href: '/settings', label: 'Settings', icon: 'M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.066 2.573c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.573 1.066c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.066-2.573c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z' },
	];

	onMount(() => {
		connectWebSocket();
	});
</script>

<div class="flex h-screen overflow-hidden">
	<!-- Sidebar -->
	<aside class="w-56 flex-shrink-0 bg-[#161B22] border-r border-[#30363D] flex flex-col {sidebarOpen ? '' : 'hidden md:flex'}">
		<!-- Logo -->
		<div class="px-4 py-4 border-b border-[#30363D]">
			<h1 class="text-lg font-bold text-[#E6EDF3]">MonDash</h1>
			<p class="text-xs text-[#8B949E]">Monitoring Dashboard</p>
		</div>

		<!-- Nav -->
		<nav class="flex-1 py-2">
			{#each navItems as item}
				<a
					href={item.href}
					class="flex items-center gap-3 px-4 py-2.5 text-sm transition-colors
						{$page.url.pathname === item.href
							? 'text-[#58A6FF] bg-[#58A6FF]/10 border-r-2 border-[#58A6FF]'
							: 'text-[#8B949E] hover:text-[#E6EDF3] hover:bg-[#1C2128]'}"
				>
					<svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" stroke-width="1.5">
						<path stroke-linecap="round" stroke-linejoin="round" d={item.icon} />
					</svg>
					{item.label}
				</a>
			{/each}
		</nav>

		<!-- ESP status -->
		<div class="px-4 py-3 border-t border-[#30363D]">
			<div class="flex items-center gap-2 text-xs">
				<span class="w-2 h-2 rounded-full {$espConnected ? 'bg-[#3FB950]' : 'bg-[#F85149] pulse-offline'}"></span>
				<span class="text-[#8B949E]">ESP32 {$espConnected ? 'Connected' : 'Disconnected'}</span>
			</div>
		</div>
	</aside>

	<!-- Main content -->
	<main class="flex-1 overflow-y-auto p-6">
		{@render children()}
	</main>
</div>

<Toast />
