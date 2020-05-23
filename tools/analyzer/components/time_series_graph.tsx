import { ResponsiveLine } from "@nivo/line";

export default function TimeSeriesGraph({ data, yLegend, colorScheme }) {
    return (
        <ResponsiveLine
            data={data}
            margin={{ top: 50, right: 110, bottom: 50, left: 60 }}
            xScale={{ type: 'point' }}
            yScale={{ type: 'linear', min: 0, max: 'auto', stacked: true }}
            axisBottom={{
                orient: 'bottom',
                tickPadding: 5,
                tickRotation: 45,
            }}
            axisLeft={{
                orient: 'left',
                tickSize: 5,
                tickPadding: 5,
                tickRotation: 0,
                legend: yLegend,
                legendOffset: -40,
                legendPosition: 'middle'
            }}
            legends={[
                {
                    anchor: "bottom-right",
                    direction: "column",
                    symbolShape: 'circle',
                    justify: true,
                    translateX: 100,
                    itemHeight: 20,
                    itemWidth: 80,
                    effects: []
                }
            ]}
            enableArea={true}
            animate={false}
            useMesh={true}
            colors={{ scheme: colorScheme ? colorScheme : "nivo" }}
        />
    )
}
