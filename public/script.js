document.getElementById('optimizeBtn').addEventListener('click', async () => {
    const sql = document.getElementById('sqlInput').value.trim();
    if (!sql) return;
    
    const errBox = document.getElementById('errorBox');
    errBox.classList.add('hidden');
    
    try {
        const response = await fetch('/api/optimize', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ query: sql })
        });
        
        const data = await response.json();
        
        if (!response.ok) {
            errBox.textContent = "Error: " + (data.error || "Failed to process query");
            errBox.classList.remove('hidden');
            return;
        }
        
        // Render Logical Tree
        const logicalMermaid = jsonToMermaid(data.logical_plan, "Logical");
        renderMermaid('logicalTree', logicalMermaid);
        
        // Render Physical Tree
        const physicalMermaid = jsonToMermaid(data.physical_plan, "Physical");
        renderMermaid('physicalTree', physicalMermaid);
        
        // Render Stats
        const statsBox = document.getElementById('statsBox');
        const statsContent = document.getElementById('statsContent');
        statsContent.innerHTML = `<p><strong>Naive Cost:</strong> Estimated unbounded execution time.</p>
                                  <p><strong>Optimized Cost:</strong> ~${data.physical_plan.cost} simulated units (Rows: ${data.physical_plan.rows || 'undefined'})</p>`;
        
        // Inject the Rewritten SQL into the UI
        if (data.optimized_sql) {
            document.getElementById('optimizedSqlText').textContent = data.optimized_sql;
        }
        
        statsBox.classList.remove('hidden');
        
    } catch (e) {
        errBox.textContent = "Network Error: Is the C++ Server running? " + e.message;
        errBox.classList.remove('hidden');
    }
});

let idCounter = 0;

// Transforms our C++ JSON Output to a Mermaid.js flowchart string
function jsonToMermaid(node, idPrefix) {
    let graph = "graph TD\n";
    
    function traverse(n, parentId) {
        if (!n) return;
        const currentId = idPrefix + "_" + (idCounter++);
        
        // Create label
        let label = n.type;
        if (n.table) label += `\\n(${n.table})`;
        if (n.predicate) label += `\\n[${n.predicate}]`;
        if (n.join_type) label += `\\n(${n.join_type})`;
        if (n.cost !== undefined) label += `\\nCost: ${n.cost}`;
        
        graph += `    ${currentId}["${label}"]\n`;
        
        if (parentId) {
            graph += `    ${parentId} --> ${currentId}\n`;
        }
        
        if (n.child) traverse(n.child, currentId);
        if (n.left) traverse(n.left, currentId);
        if (n.right) traverse(n.right, currentId);
    }
    
    traverse(node, null);
    
    // Add some styling
    graph += `    classDef default fill:#1f242e,stroke:#30363d,stroke-width:2px,color:#c9d1d9;\n`;
    return graph;
}

async function renderMermaid(containerId, mermaidText) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    const { svg } = await window.mermaid.render(`mermaid-${containerId}-${Date.now()}`, mermaidText);
    container.innerHTML = svg;
}