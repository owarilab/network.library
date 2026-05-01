# Conditional Component Implementation Guide

## Overview

The `Conditional` component enables complex branching logic in game events. It evaluates multiple conditions in order and executes the action of the first matching branch (short-circuit evaluation).

**Key Differences from EventAction**:
- `EventAction`: Single if/then/else logic
- `Conditional`: Multiple if/elseif/else branches with short-circuit evaluation

## Architecture

### Component Structure

```
Conditional Component
├── listenTo: event ID
├── branches: Branch[]
│   └── Branch
│       ├── condition: Condition
│       └── action: EventAction.data
└── defaultAction: EventAction.data (optional)
```

### Condition Types

| Type | Purpose | Parameters |
|------|---------|------------|
| compare | Numeric comparison | left, operator (>, >=, <, <=, ===, !==), right |
| equals | String exact match | left, right (case-sensitive) |
| truthy | JavaScript truthiness | left (falsy: false, 0, "", null, undefined) |
| has | Object property exists | left (object/variable), right (property name) |
| exists | Variable exists | left (variable path string, no ${}) |

## Usage Examples

### Example 1: RPG Battle AI

Enemy decides action based on HP:

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_enemy_turn",
    "branches": [
      {
        "condition": {
          "type": "compare",
          "left": "${user.persistent.enemyHp}",
          "operator": "<",
          "right": 15
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.battleAction",
          "valueSource": "literal",
          "value": "FLEE",
          "op": "set"
        }
      },
      {
        "condition": {
          "type": "compare",
          "left": "${user.persistent.hp}",
          "operator": ">",
          "right": 80
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.battleAction",
          "valueSource": "literal",
          "value": "ATTACK",
          "op": "set"
        }
      }
    ],
    "defaultAction": {
      "action": "setGlobalVariable",
      "variablePath": "user.persistent.battleAction",
      "valueSource": "literal",
      "value": "DEFEND",
      "op": "set"
    }
  }
}
```

### Example 2: Item Usage Logic

Decide which item to use based on available inventory:

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_use_item",
    "branches": [
      {
        "condition": {
          "type": "has",
          "left": "${user.persistent.inventory}",
          "right": "antidote"
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.itemUsed",
          "valueSource": "literal",
          "value": "antidote",
          "op": "set"
        }
      },
      {
        "condition": {
          "type": "has",
          "left": "${user.persistent.inventory}",
          "right": "potion"
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.itemUsed",
          "valueSource": "literal",
          "value": "potion",
          "op": "set"
        }
      }
    ],
    "defaultAction": {
      "action": "setGlobalVariable",
      "variablePath": "user.persistent.itemUsed",
      "valueSource": "literal",
      "value": "nothing",
      "op": "set"
    }
  }
}
```

### Example 3: State Machine

Transition based on current state and condition:

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_state_update",
    "branches": [
      {
        "condition": {
          "type": "equals",
          "left": "${user.persistent.state}",
          "right": "poisoned"
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.hp",
          "valueSource": "literal",
          "value": 1,
          "op": "subtract"
        }
      },
      {
        "condition": {
          "type": "equals",
          "left": "${user.persistent.state}",
          "right": "regenerating"
        },
        "action": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.hp",
          "valueSource": "literal",
          "value": 2,
          "op": "add"
        }
      }
    ],
    "defaultAction": {
      "action": "setGlobalVariable",
      "variablePath": "user.persistent.state",
      "valueSource": "literal",
      "value": "normal",
      "op": "set"
    }
  }
}
```

## Implementation Details

### Evaluation Flow

```
1. Event "listenTo" is fired
2. _executeConditional() is called
3. For each branch in order:
   a. Evaluate condition using ConditionalAction.evaluateCondition()
   b. If true:
      - Execute branch.action
      - Set status message
      - RETURN (short-circuit, no more branches)
4. If no branch matched and defaultAction exists:
   - Execute defaultAction
   - Set status message
5. Otherwise:
   - No matching branch, no defaultAction
   - Set info status message
```

### Code Integration Points

**in play_test_scene.js:**

1. `_processEventActions()` detects Conditional components:
   ```javascript
   if (component.type === 'Conditional') {
     this._executeConditional(playUnit, component.data);
   }
   ```

2. `_executeConditional()` evaluates conditions:
   ```javascript
   const evaluated = ConditionalAction.evaluateCondition(condition, this._appData);
   if (evaluated) {
     this._executeAction(playUnit, branch.action);
     return;
   }
   ```

**in conditional_action.js:**

- `evaluateCondition(condition, appData)`: Main dispatcher
  - Detects condition type
  - Calls appropriate evaluation method
  - Returns boolean result

- `_resolveValue(value, appData)`: Template variable expansion
  - Detects `${variable_path}` patterns
  - Calls `appData.getRuntimeGlobalVariable()`
  - Returns resolved value or literal

### Template Variable Syntax

Template variables work in condition values (both `left` and `right`):

```json
{
  "type": "compare",
  "left": "${user.persistent.hp}",
  "operator": ">",
  "right": 50
}
```

**Resolution Process**:
1. Detect `${...}` pattern
2. Extract variable path
3. Call `appData.getRuntimeGlobalVariable(path)`
4. Return numeric value, or undefined if not found
5. Fallback: Use literal value if no `${}` detected

## Condition Type Details

### compare
- **Operators**: `>`, `>=`, `<`, `<=`, `===`, `!==`
- **Types**: Numeric comparison (string-to-number coercion)
- **Fallback**: NaN comparisons return false
- **Example**:
  ```json
  {
    "type": "compare",
    "left": "${user.persistent.hp}",
    "operator": ">=",
    "right": 50
  }
  ```

### equals
- **Comparison**: String exact match, case-sensitive
- **Example**:
  ```json
  {
    "type": "equals",
    "left": "${user.persistent.state}",
    "right": "defending"
  }
  ```

### truthy
- **Falsy Values**: false, 0, "", null, undefined
- **Truthy Values**: Everything else
- **Example**:
  ```json
  {
    "type": "truthy",
    "left": "${user.persistent.hasShield}"
  }
  ```

### has
- **Purpose**: Check if object has a property
- **Requires**: left must resolve to an object
- **Example**:
  ```json
  {
    "type": "has",
    "left": "${user.persistent.inventory}",
    "right": "potion"
  }
  ```

### exists
- **Purpose**: Check if global variable path exists
- **Note**: `left` is a variable path STRING, not wrapped in `${}`
- **Example**:
  ```json
  {
    "type": "exists",
    "left": "user.persistent.hp"
  }
  ```

## Testing

### Unit Tests (conditional_action_test.js)

- 31 test cases covering all 5 condition types
- Edge cases: NaN, Infinity, null, undefined, empty strings
- Template variable expansion tests
- Error handling verification

### Integration Tests (test_conditional_demo.qsproj)

**PlayUnit 1: ConditionalTest**
- 6 basic condition type tests
- Result variables display test results
- Templates used for dynamic result display

**PlayUnit 2: RPG Battle AI**
- Complex scenario with multiple branches
- Demonstrates short-circuit evaluation
- Real-world use case: enemy AI decision-making

## Debugging

### Status Messages

The implementation logs status messages to help debug:

```javascript
// Successful branch execution
"Conditional: executed branch with action 'setGlobalVariable'"

// No matching branch
"Conditional: no matching branch, no defaultAction"

// Missing branches array
"Conditional: branches array is missing"
```

### Console Logging

`ConditionalAction` logs warnings for invalid conditions:
```javascript
console.warn(`Condition evaluation error: ${errorMessage}`);
```

## Performance Considerations

- **Short-circuit**: Evaluation stops at first true condition (efficient)
- **Branch Order**: Place most common/fastest conditions first
- **Template Resolution**: Happens per-frame for dynamic values
- **Memory**: Each branch creates one condition object during evaluation

## Future Extensions

Possible enhancements:
- Logical operators: `AND`, `OR`, `NOT`
- Nested conditions for complex boolean logic
- Arithmetic in comparisons: `left1 + left2 > right`
- Regex matching for equals condition
- Custom condition types via plugin system
