<#-- @ftlvariable name="data" type="ae.hitb.proctf.drone_racing.IndexData" -->
<html>
    <body>
        <ul>
        <#list data.items as item>
            <li>${item}</li>
        </#list>
        </ul>
    </body>
</html>
