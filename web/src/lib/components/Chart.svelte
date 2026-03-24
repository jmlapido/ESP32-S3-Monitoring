<script lang="ts">
	import { onMount } from 'svelte';
	import { Chart, registerables } from 'chart.js';

	let { data = [], labels = [], title = 'Chart' }: { data: number[]; labels: (string | number)[]; title: string } = $props();

	let canvas: HTMLCanvasElement;
	let chart: Chart | null = null;

	Chart.register(...registerables);

	onMount(() => {
		chart = new Chart(canvas, {
			type: 'line',
			data: {
				labels,
				datasets: [{
					label: title,
					data,
					borderColor: '#58A6FF',
					backgroundColor: 'rgba(88, 166, 255, 0.1)',
					fill: true,
					tension: 0.3,
					pointRadius: 0,
					borderWidth: 2,
				}],
			},
			options: {
				responsive: true,
				maintainAspectRatio: false,
				plugins: {
					legend: { display: false },
					tooltip: {
						backgroundColor: '#1C2128',
						titleColor: '#E6EDF3',
						bodyColor: '#8B949E',
						borderColor: '#30363D',
						borderWidth: 1,
					},
				},
				scales: {
					x: {
						grid: { color: '#30363D' },
						ticks: { color: '#8B949E', maxTicksLimit: 8 },
					},
					y: {
						grid: { color: '#30363D' },
						ticks: { color: '#8B949E' },
					},
				},
			},
		});

		return () => chart?.destroy();
	});

	$effect(() => {
		if (chart) {
			chart.data.labels = labels;
			chart.data.datasets[0].data = data;
			chart.update('none');
		}
	});
</script>

<div class="w-full h-64 bg-[#161B22] rounded-lg border border-[#30363D] p-4">
	<canvas bind:this={canvas}></canvas>
</div>
